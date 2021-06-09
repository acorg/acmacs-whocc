$(document).ready(() => {
    console.log(index_subtypes);
    const subtype_tabs = $("<div class='subtype-tabs'></div>").appendTo("body");
    const tablinks = $("<div></div>").addClass("tablinks").prependTo(subtype_tabs);
    for (let subtype_data of index_subtypes) {
        const tabcontent = $(`<div>${subtype_data.id}</div>`).addClass("tabcontent").appendTo(subtype_tabs);
        const button = $("<button></button>")
              .addClass("tablink")
              .attr("id", `button-${subtype_data.id}`)
              .html(subtype_data.id)
              .on("click", ev => {
                  subtype_tabs.children(".tabcontent").each((_, tc) => tc.style.display = "none");
                  subtype_tabs.find(".tablink").each((_, tl) => tl.classList.remove("active"));
                  tabcontent.show();
                  ev.currentTarget.classList.add("active");
              }).appendTo(tablinks);
    }
    $(`#button-${index_subtypes[0].id}`).click();
});

