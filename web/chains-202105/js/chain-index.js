function show_subtype_tabs() {
    console.log(index_subtypes);
    // split_subtypes(index_subtypes);
    // sort split_subtypes keys
    index_subtypes = sort_subtypes(index_subtypes);
    const subtype_tabs = $("<div class='subtype-tabs'></div>").appendTo("body");
    const tablinks = $("<div></div>").addClass("tablinks").prependTo(subtype_tabs);
    for (let subtype_data of index_subtypes) {
        const title = subtype_tab_title(subtype_data);
        const tabcontent = $("<div></div>")
              .addClass("tabcontent")
              .append(`<div class='loading-message'>Loading ${title}, please wait</div>`)
              .appendTo(subtype_tabs);
        load_subtype_tab_data(tabcontent, subtype_data);
        const button = $("<button></button>")
              .addClass("tablink")
              .attr("id", `button-${subtype_data.id}`)
              .html(title)
              .on("click", ev => {
                  subtype_tabs.children(".tabcontent").each((_, tc) => tc.style.display = "none");
                  subtype_tabs.find(".tablink").each((_, tl) => tl.classList.remove("active"));
                  tabcontent.show();
                  ev.currentTarget.classList.add("active");
              }).appendTo(tablinks);
    }
    $(`#button-${index_subtypes[0].id}`).click();
}

// ----------------------------------------------------------------------

function load_subtype_tab_data(tabcontent, subtype_data) {
    $.getJSON(`api/subtype-data/?subtype_id=${subtype_data.id}`, (data) => {
    // $.getJSON(`api/subtype-data`, (data) => {
        console.log(subtype_data.id, data);
    });
}

// ----------------------------------------------------------------------

function subtype_tab_title(subtype_data) {
    if (subtype_data.subtype === "h3")
        return `${dir_subtype_to_display[subtype_data.subtype]} ${dir_assay_to_display[dir_assay_to_assay[subtype_data.assay]]}`;
    else
        return dir_subtype_to_display[subtype_data.subtype];
}

function split_subtypes(subtypes) {
    const subtype_assays = {};
    for (let subtype_data of subtypes) {
        const subtype_assay = subtype_data.subtype + "-" + dir_assay_to_assay[subtype_data.assay];
        if (!subtype_assays[subtype_assay])
            subtype_assays[subtype_assay] = [subtype_data];
        else
            subtype_assays[subtype_assay].push(subtype_data);
    }
    return subtype_assays;
}


function sort_subtypes(subtypes) {
    const subtype_order = ["h1pdm", "h3", "bvic", "byam"];
    const assay_order = ["hint", "neut", "hi"];
    const rank = (subtype_data) => subtype_order.indexOf(subtype_data.subtype) * 100 + assay_order.indexOf(subtype_data.assay);
    return subtypes.sort((en1, en2) => { return rank(en1) - rank(en2); });
}

// ----------------------------------------------------------------------

$(document).ready(() => show_subtype_tabs());

