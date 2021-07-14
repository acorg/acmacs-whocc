import * as DIR from "./directories.js";

// ----------------------------------------------------------------------

function main() {
    console.log(table_page_data);
    make_title()
    $("body").append(`<pre>\n${JSON.stringify(table_page_data, null, 2)}\n</pre>`)
}

// ----------------------------------------------------------------------

function make_title() {
    if (table_page_data.subtype === "h3")
        $("#title").html(`${DIR.lab_to_display[table_page_data.lab]} ${DIR.subtype_to_display[table_page_data.subtype]} ${DIR.assay_to_display[table_page_data.assay]} ${table_page_data.rbc || ""} ${table_page_data.table_date}`)
    else
        $("#title").html(`${DIR.lab_to_display[table_page_data.lab]} ${DIR.subtype_to_display[table_page_data.subtype]} ${table_page_data.table_date}`)
}

// ----------------------------------------------------------------------

$(document).ready(() => main());
