import * as DIR from "./directories.js";
import * as MAPS from "./maps.js";

// ----------------------------------------------------------------------

function main() {
    console.log("table_page_data", table_page_data);
    make_title();
    for (let part_data of table_page_data.parts)
        show_part(part_data, table_page_data.subtype_id);
    // $("body").append(`<pre>\n${JSON.stringify(table_page_data, null, 2)}\n</pre>`);
}

// ----------------------------------------------------------------------

function show_part(part_data, subtype_id) {
    const part_title_text = part_title(part_data, subtype_id);
    if (part_title_text)
        $("body").append(`<hr>\n<h3>${part_title_text} ${part_data.date} (${part_data.chain_id})</h3>`);
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
    table_page_data.coloring.forEach((coloring, coloring_no) => {
        const req = MAPS.make_request_data({type: "map", ace: data.ace, coloring: coloring, size: MAPS.IMAGE_SIZE, save_chart: coloring_no === 0})
        const link = MAPS.make_link({ace: data.ace})
        tr.append(`<td><a href="${link}" target="_blank"><img src="png?${req}"></a></td>`);
    });
    // TODO: grid test
    // TODO: serum correlation widget
    // TODO: table widget
}

// ----------------------------------------------------------------------

function show_chain_maps(data) {
    const table = $("<table></table>").appendTo("body");
    table_page_data.coloring.forEach((coloring, coloring_no) => {
        const tr_title = $("<tr class='title'></tr>").appendTo(table);
        const tr = $("<tr class='image'></tr>").appendTo(table);
        for (let merge_type of ["incremental", "scratch"]) {
            const td_title = MAPS.map_td_with_title(data, merge_type, coloring, coloring_no === 0);
            if (td_title) {
                tr_title.append(td_title.title);
                tr.append(td_title.td);
            }
        }
        // TODO: pc incremental vs. scratch
        for (let merge_type of ["incremental", "scratch"]) {
            // TODO: grid test
        }
    });
}

// ----------------------------------------------------------------------

function part_title(part_data, subtype_id) {
    switch (part_data.type) {
    case "individual":
        return null;
    case "chain":
        switch (part_data.chain_id[0]) {
        case "f":
            return `<a href="chain?subtype_id=${subtype_id}&chain_id=${part_data.chain_id}" target="_blank">Chain</a>`;
        case "b":
            return `<a href="chain?subtype_id=${subtype_id}&chain_id=${part_data.chain_id}" target="_blank">Backward chain</a>`;
        }
        break;
    }
    return null;
}

// ----------------------------------------------------------------------

function make_title() {
    let prefix = `${DIR.lab_to_display[table_page_data.lab]} ${DIR.subtype_to_display[table_page_data.subtype]}`;
    if (table_page_data.subtype === "h3")
        prefix += ` ${DIR.assay_to_display[table_page_data.assay]} ${table_page_data.rbc || ""}`;
    $("#title").html(`${prefix} ${table_page_data.table_date}`);
}

// ----------------------------------------------------------------------

$(document).ready(() => main());
