// gridage v1 2020-10-13

const isSafari = /^((?!chrome|android).)*safari/i.test(navigator.userAgent);

// ----------------------------------------------------------------------

async function main()
{
    const response = await fetch("gridage.json");
    if (response.ok) {
        let data = await response.json();
        console.log(data);

        make_title(data);
        make_image_resizer();
        make_page(data);
    }
    else {
        document.body.append(make_element("h1", {content: "error loading gridage.json: " + response.status}));
    }
}

// ----------------------------------------------------------------------

function make_title(data)
{
    set_head_title(data.title?.short || data.title?.long || "");
    const title_date = data.title?.date ? ` (${data.title.date})` : "";
    document.body.append(make_element("h1", {content: `${data.title?.long}${title_date}`}));
}

// ----------------------------------------------------------------------

function make_image_resizer()
{
    const resizer_div = make_element("div", {append_to: document.body, klass: "image-resizer"});
    const label = make_element("div", {content: "Image size", klass: "label", append_to: resizer_div});
    const resizer = make_element("input", {attrs: {type: "range", min: "200", max: "4000", name: "resizer"}, append_to: resizer_div});
    const resize_images = function(ev) {
        const var_name = "--grid-image-size";
        if (!ev)
            resizer.value = parseInt(get_css_root_var(var_name));
        set_css_root_var(var_name, resizer.value + "px");
        label.innerHTML = "Image size: " + resizer.value;
    };
    resizer.oninput = resize_images;
    resize_images();
}

// ----------------------------------------------------------------------

function get_css_root_var(name)
{
    for (const rule of document.styleSheets[0].rules) {
        if (rule.selectorText == ":root")
            return rule.style.getPropertyValue(name);
    }
    return undefined;
}

function set_css_root_var(name, value)
{
    for (const rule of document.styleSheets[0].rules) {
        if (rule.selectorText == ":root") {
            rule.style.setProperty(name, value);
            break;
        }
    }
}

// ----------------------------------------------------------------------

function make_page(data)
{
    const ol = make_element("ol", {append_to: document.body, klass: "main"});
    for (let row_entry of (data.page || [])) {
        const li = make_element("li", {append_to: ol});
        li.append(make_element("span", {content: row_entry.title, klass: "section-title"}));
        li.append(make_element("div", {klass: "section-title-separator"}));
        if (row_entry.columns?.length) {
            const max_columns = Math.max(... row_entry.columns.map(col => col.length))
            const div_grid = make_element("div", {append_to: li, klass: ["grid", `grid-${max_columns}`]});
            for (let column_entry of row_entry.columns) {
                for (let row of column_entry) {
                switch (row.T) {
                case "title":
                    make_element("div", {append_to: div_grid, content: row.text, klass: "title"});
                    break;
                case "pdf":
                    make_pdf({append_to: div_grid, src: row.file});
                    break;
                }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------

function make_pdf({append_to, src})
{
    if (isSafari) {
        make_element("img", {attrs: {src: src}, append_to: append_to, klass: "pdf"});
    }
    else {
        // https://www.adobe.com/content/dam/acom/en/devnet/acrobat/pdfs/pdf_open_parameters.pdf
        make_element("object", {attrs: {data: `${src}#toolbar=0&view=Fit`, type: "application/pdf"}, append_to: append_to, klass: "pdf"});
    }
}

function make_element(type, {content, append_to, klass, attrs} = {})
{
    let elt = document.createElement(type);
    if (content)
        elt.append(content);
    if (append_to)
        append_to.append(elt);
    if (Array.isArray(klass)) {
        for (let kl of klass)
            elt.classList.add(kl);
    }
    else if (typeof klass === "string") {
        elt.classList.add(klass);
    }
    if (attrs) {
        for (const [key, value] of Object.entries(attrs))
            elt.setAttribute(key, value);
    }
    return elt;
}

function set_head_title(text)
{
    let elts = document.head.getElementsByTagName("title");
    if (elts.length == 0) {
        document.head.append(make_element("title", {content: text}));
    }
    else {
        elts.item(0).textContent = text;
    }
}

// ----------------------------------------------------------------------

document.addEventListener("DOMContentLoaded", main);

