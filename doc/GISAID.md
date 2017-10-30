### Gisaid

  - visit [http://gisaid.org](http://gisaid.org) and login (at right)
  - Click EpiFlu (top middle)
  - Hold Cmd and click A and B in the Type.
  - Set collection date From to 2017-01-01 (or later depending on the number of isolates upon search, see below)
  - _Leave originating lab empty!_

  - Select **Submitting lab** them all using Cmd

    (Change height of the submitting lab <select> 500px)

         [Australia, North Melbourne] WHO CC ...
         [United Kingdom, London] National Institute for Medical Research
         [United Kingdom, London] Crick
         [United states, Atlanta] CDC
         ? [China, Bejing] WHO Chinese National Influenza Center

  - Required segments: HA
  - Click Search. Total must not exceed 20k isolates (gisaid does not allow downloading more)
  - Click leftmost checkbox in header to select all
  - Click Download
  - Format: Sequences (DNA) as FASTA
  - Segment: HA
  - FASTA Header: Isolate name |  Collection date | Passage details/history |  Sample ID by submitting lab | Submitting lab | Type |  Lineage
  - Date format: YYYY-MM-DD
  - Click download

  - Rename file:

        dos2unix ~/Downloads/gisaid_epiflu_sequence.fasta
        mv ~/Downloads/gisaid_epiflu_sequence.fasta ~/Downloads/gisaid-all-<start-date>-$(date +%Y%m%d).fas
        bzip2 -9v ~/Downloads/gisaid-all-<start-date>-$(date +%Y%m%d).fas
        mv ~/Downloads/gisaid-all-<start-date>-$(date +%Y%m%d).fas.bz2 ~/ac/tables-store/sequences
        ssax
