import * as DIR from "./directories.js";
import * as MAPS from "./maps.js";

// ----------------------------------------------------------------------

function main() {
    console.log("chain_page_data", chain_page_data);
    make_title();
    for (let part_data of chain_page_data.parts)
        show_part(part_data, chain_page_data.subtype_id);
}

// ----------------------------------------------------------------------

function show_part(part_data, subtype_id) {
    const part_title_text = part_title(part_data, subtype_id);
    if (part_title_text)
        $("body").append(`<hr>\n<h3>${part_title_text}</h3>`);
    show_maps(part_data);
}

// ----------------------------------------------------------------------

function show_maps(data) {
    const table = $("<table></table>").appendTo("body");
    chain_page_data.coloring.forEach((coloring, coloring_no) => {
        const tr_title = $("<tr></tr>").appendTo(table);
        const tr = $("<tr></tr>").appendTo(table);
        for (let merge_type of ["incremental", "scratch", "individual", "mcb"]) {
            if (data[merge_type] && data[merge_type].ace) {
                tr_title.append(`<td>${merge_type}</td>`);
                const req = MAPS.make_request_data({type: "map", ace: data[merge_type].ace, coloring: coloring, size: MAPS.IMAGE_SIZE, save_chart: coloring_no === 0})
                const link = MAPS.make_link({ace: data[merge_type].ace})
                tr.append(`<td><a href="${link}" target="blank_"><img src="png?${req}"></a></td>`);
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
    return `${part_data.step} ${part_data.date}`;
}

// ----------------------------------------------------------------------

function make_title() {
    let prefix = `${DIR.lab_to_display[chain_page_data.lab]} ${DIR.subtype_to_display[chain_page_data.subtype]}`;
    if (chain_page_data.subtype === "h3")
        prefix += ` ${DIR.assay_to_display[chain_page_data.assay]} ${chain_page_data.rbc || ""}`;
    const dates = `${chain_page_data.parts[chain_page_data.parts.length - 1].date}-${chain_page_data.parts[0].date}`;
    $("#title").html(`${prefix}${chain_page_data.type}: ${dates} ${chain_page_data.parts.length} tables (${chain_page_data.chain_id})`);
}

// ----------------------------------------------------------------------

$(document).ready(() => main());
