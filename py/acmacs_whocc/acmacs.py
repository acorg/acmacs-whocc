import sys, subprocess, pprint
from pathlib import Path
import logging; module_logger = logging.getLogger(__name__)
# from . import error, utility
from acmacs_base import json, files

# ----------------------------------------------------------------------

sVirusTypeConvert = {
    'A(H1N1)': 'h1seas',
    'A(H1N1)2009PDM': 'h1pdm',
    'A(H3N2)': 'h3',
    'BYAMAGATA': 'b-yam',
    'BVICTORIA': 'b-vic',
    }

sVirusTypeConvert_ssm_report = {
    'A(H1N1)': None,
    'A(H1N1)2009PDM': 'h1',
    'A(H3N2)': 'h3',
    'BYAMAGATA': 'by',
    'BVICTORIA': 'bv',
    }

sAssayConvert = {
    "HI": "hi",
    "FOCUS REDUCTION": "neut",
    "PLAQUE REDUCTION NEUTRALISATION": "neut",
    "MN": "neut",
    }

# ----------------------------------------------------------------------

def get_recent_merges(target_dir :Path, subtype=None, lab=None):
    if subtype is not None:
        response = api().command(C="ad_whocc_recent_merges", log=False, virus_types=subtype, labs = [lab.upper()] if lab else None)
        if "data" not in response:
            module_logger.error("No \"data\" in response of ad_whocc_recent_merges api command:\n{}".format(pprint.pformat(response)))
            raise RuntimeError("Unexpected result of ad_whocc_recent_merges c2 api command")
        response = response['data']
        response.sort(key=lambda e: "{lab:4s} {virus_type:10s} {assay}".format(**e))
        module_logger.info('WHO CC recent merges\n{}'.format("\n".join("{lab:4s} {virus_type:14s} {assay:31s} {chart_id}".format(**e) for e in response)))
        for entry in response:
            if entry["lab"] != "CNIC" and not (entry["lab"] == "NIID" and entry["virus_type"] == "A(H3N2)" and entry["assay"] == "HI"):
                basename = f"{entry['lab'].lower()}-{subtype}-{sAssayConvert[entry['assay']].lower()}"
                filename = target_dir.joinpath(f"{basename}.ace")
                chart = api().command(C="chart_export", log=False, id=entry["chart_id"], format="ace", part="chart")["chart"]
                if isinstance(chart, dict) and "##bin" in chart:
                    files.backup_file(filename)
                    module_logger.info(f"writing {filename}")
                    import base64
                    filename.open('wb').write(base64.b64decode(chart["##bin"].encode('ascii')))
    else:
        for subtype in ["h1", "h3", "bv", "by"]:
            get_recent_merges(target_dir=target_dir, subtype=subtype)

# ----------------------------------------------------------------------

sAPI = None

def api():
    global sAPI
    if sAPI is None:
        sAPI = API(session=subprocess.check_output(["aw-session", "whocc-viewer"]).decode("utf-8").strip())
    return sAPI

# ----------------------------------------------------------------------

class API:

    def __init__(self, session=None, user="whocc-viewer", password=None, url_prefix='https://acmacs-web.antigenic-cartography.org/'):
        """If host is None, execute command in this acmacs instance directly."""
        self.url_prefix = url_prefix
        self.session = session
        if not session and user:
            self._login(user, password)

    def _execute(self, command, print_response=False, log_error=True, raise_error=False):
        if self.url_prefix:
            if self.session:
                command.setdefault('S', self.session)
            response = self._execute_http(command)
        else:
            raise NotImplementedError()
            # ip_address = '127.0.0.1'
            # command.setdefault('I', ip_address)
            # command.setdefault('F', 'python')
            # if self.session:
            #     from ..mongodb_collections import mongodb_collections
            #     command.setdefault('S', mongodb_collections.sessions.find(session_id=self.session, ip_address=ip_address))
            # from .command import execute
            # response = execute(command)
            # if isinstance(response.output, str):
            #     response.output = json.loads(response.output)
        #module_logger.info(repr(response.output))
        if isinstance(response, dict) and response.get('E'):
            if log_error:
                module_logger.error(response['E'])
                for err in response['E']:
                    if err.get('traceback'):
                        module_logger.error(err['traceback'])
            if raise_error:
                raise CommandError(response['E'])
        elif print_response:
            if isinstance(response, dict) and response.get('help'):
                module_logger.info(response['help'])
            else:
                module_logger.info('{} {!r}'.format(type(response), response))
        return response

    def _execute_http(self, command):
        command['F'] = 'json'
        module_logger.debug('_execute_http %r', command)
        response = self._urlopen(url='{}/api'.format(self.url_prefix), data=json.dumps(command).encode('utf-8'))
        return json.loads(response)

    def _login(self, user, password):
        import random
        response = self._execute(command=dict(F='python', C='login_nonce', user=user), print_response=False)
        if response.get('E'):
            raise LoginFailed(response['E'])
        # module_logger.debug('login_nonce user:{} nonce:{}'.format(user, response))
        digest = self._hash_password(user=user, password=password)
        cnonce = '{:X}'.format(random.randrange(0xFFFFFFFF))
        password = self._hash_nonce_digest(nonce=response['nonce'], cnonce=cnonce, digest=digest)
        response = self._execute(command=dict(F='python', C='login', user=user, cnonce=cnonce, password=password, application=sys.argv[0]), print_response=False)
        module_logger.debug('response {}'.format(response))
        if response.get('E'):
            raise LoginFailed(response['E'])
        self.session = response['S']
        module_logger.info('--session={}'.format(self.session))

    def command(self, C, print_response=False, log_error=True, raise_error=False, **args):
        cmd = dict(C=C, log_error=log_error, **self._fix_args(args))
        # try:
        #     getattr(self, '_fix_command_' + C)(cmd)
        # except AttributeError:
        #     pass
        return self._execute(cmd, print_response=print_response, log_error=log_error, raise_error=raise_error)

    def download(self, id):
        return self._urlopen(url='{}/api/?cache=1&session={}&id={}'.format(self.url_prefix, self.session, id))

    def _urlopen(self, url, data=None):
        import ssl, urllib.request
        context = ssl.create_default_context()
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
        return urllib.request.urlopen(url=url, data=data, context=context).read()

    def _hash_password(self, user, password):
        import hashlib
        m = hashlib.md5()
        m.update(';'.join((user, 'acmacs-web', password)).encode('utf-8'))
        return m.hexdigest()

    def _hash_nonce_digest(self, nonce, cnonce, digest):
        import hashlib
        m = hashlib.md5()
        m.update(';'.join((nonce, cnonce, digest)).encode('utf-8'))
        return m.hexdigest()

    def __getattr__(self, name):
        if name[0] != '_':
            return lambda **a: self.command(name, **a)
        else:
            raise AttributeError(name)

    # def _fix_command_chart_new(self, cmd):
    #     if isinstance(cmd['chart'], str) and os.path.isfile(cmd['chart']): # read data from filename and encode it to make json serializable
    #         cmd['chart'] = json.BinaryData(open(cmd['chart'], 'rb').read())

    def _fix_args(self, args):
        for to_int in ('skip', 'max_results', 'size'):
            if args.get(to_int) is not None:
                args[to_int] = int(args[to_int])
        return args

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
