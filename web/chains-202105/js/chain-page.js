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
    // const part_title_text = part_title(part_data, subtype_id);
    // if (part_title_text)
    //     $("body").append(`<hr>\n<h3>${part_title_text} ${part_data.date} (${part_data.chain_id})</h3>`);
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

function make_title() {
    if (chain_page_data.subtype === "h3")
        $("#title").html(`${DIR.lab_to_display[chain_page_data.lab]} ${DIR.subtype_to_display[chain_page_data.subtype]} ${DIR.assay_to_display[chain_page_data.assay]} ${chain_page_data.rbc || ""} ${chain_page_data.chain_id}`);
    else
        $("#title").html(`${DIR.lab_to_display[chain_page_data.lab]} ${DIR.subtype_to_display[chain_page_data.subtype]} ${chain_page_data.chain_id}`);
}

// ----------------------------------------------------------------------

$(document).ready(() => main());
