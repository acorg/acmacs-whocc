import * as DIR from "./directories.js";

const IMAGE_SIZE = 800;

// ----------------------------------------------------------------------

function main() {
    console.log(table_page_data);
    make_title();
    for (let part_data of table_page_data.parts)
        show_part(part_data);
    $("body").append(`<pre>\n${JSON.stringify(table_page_data, null, 2)}\n</pre>`);
}

// ----------------------------------------------------------------------

function show_part(part_data) {
    const part_title_text = part_title(part_data);
    if (part_title_text)
        $("body").append(`<h3>${part_title_text} ${part_data.date} (${part_data.chain_id})</h3>`);
    switch (part_data.type) {
    case "individual":
        show_individual_table_maps(part_data.scratch);
        break;
    case "chain":
        show_chain_maps(part_data);
        break;
    }

}

// ----------------------------------------------------------------------

function show_individual_table_maps(data) {
    const tr = $("<table><tr></tr></table>").appendTo("body").find("tr");
    for (let coloring of table_page_data.coloring) {
        const req = make_request_data({type: "map", ace: data.ace, coloring: coloring, size: IMAGE_SIZE})
        tr.append(`<td><a href=""><img src="png?${req}"></a></td>`);
    }
    // grid test
    // serum correlation widget
    // table widget
}

// ----------------------------------------------------------------------

function show_chain_maps(data) {
}

// ----------------------------------------------------------------------

function make_request_data(data) {
    return Object.entries(data).map((en) => `${en[0]}=${encodeURIComponent(en[1])}`).join('&')
}

// ----------------------------------------------------------------------

function part_title(part_data) {
    switch (part_data.type) {
    case "individual":
        return null;
    case "chain":
        switch (part_data.chain_id[0]) {
        case "f":
            return "Chain";
        case "b":
            return "Backward chain";
        }
        break;
    }
    return null;
}

// ----------------------------------------------------------------------

function make_title() {
    if (table_page_data.subtype === "h3")
        $("#title").html(`${DIR.lab_to_display[table_page_data.lab]} ${DIR.subtype_to_display[table_page_data.subtype]} ${DIR.assay_to_display[table_page_data.assay]} ${table_page_data.rbc || ""} ${table_page_data.table_date}`);
    else
        $("#title").html(`${DIR.lab_to_display[table_page_data.lab]} ${DIR.subtype_to_display[table_page_data.subtype]} ${table_page_data.table_date}`);
}

// ----------------------------------------------------------------------

$(document).ready(() => main());
