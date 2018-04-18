import {draw} from "../draw/acmacs-draw-surface.js";

export function main(wide_page_data) {
    console.log(wide_page_data);
    const start = new Date();

    $("body").append("<table><tr class='column_names'></tr><tr class='head_individual'></tr><tr class='individual'></tr><tr class='head_incremental'></tr><tr class='incremental'></tr><tr class='head_scratch'></tr><tr class='scratch'></tr></table>");
    wide_page_data.maps.forEach(entry => {
        $("tr.column_names").append("<td>" + entry["1"].name + "</td");
        add_column(entry["1"], "individual");
        add_column(entry.i, "incremental");
        add_column(entry.s, "scratch");
    });
    $("body").append("<p class='chain_id'>Chain: " + wide_page_data.chain_id + "</p>");
    $("body").append("<p class='modified'>" + wide_page_data.generated + "</p>");
    const elapsed = new Date() - start;
    console.log("wide page drawing time: " + elapsed + "ms -> " + (1000 / elapsed).toFixed(1) + "fps");
}

// ----------------------------------------------------------------------

function add_column(entry, tag) {
    if (entry) {
        $("tr.head_" + tag).append("<td>" + entry.stress.toFixed(2) + "</td");
        $("tr." + tag).append("<td><canvas id='" + entry.chart_id + "' /></td>");
        draw_on(entry.chart_id);
    }
    else {
        $("tr.head_" + tag).append("<td></td>");
        $("tr." + tag).append("<td></td>");
    }
}

function draw_on(chart_id) {
    let settings = {canvas: {width: 300, height: 300}, point_scale: 1};
    $.getJSON(chart_id + ".map.json", function(map_data) {
        draw($("#" + chart_id), [map_data, settings]);
    });
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
