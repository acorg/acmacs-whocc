import * as DIR from "./directories.js";

const IMAGE_SIZE = 800;

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
    // switch (part_data.type) {
    // case "individual":
    //     show_individual_table_maps(part_data.scratch);
    //     break;
    // case "chain":
    //     show_chain_maps(part_data);
    //     break;
    // }
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
    $("#title").html(`${prefix} ${chain_page_data.type}: ${dates} ${chain_page_data.parts.length} tables (${chain_page_data.chain_id})`);
}

// ----------------------------------------------------------------------

$(document).ready(() => main());
