import {draw} from "./acmacs-draw-surface.js";

export function main(wide_page_data) {
    console.log(wide_page_data);
    const start = new Date();

    $("body").append("<table><tr class='column_names'></tr><tr class='head_individual'></tr><tr class='individual'></tr><tr class='head_incremental'></tr><tr class='incremental'></tr><tr class='head_scratch'></tr><tr class='scratch'></tr></table>");
    wide_page_data.forEach(entry => {
        $("tr.column_names").append("<td>" + entry["1"].name + "</td");
        $("tr.head_individual").append("<td>" + (entry["1"] ? entry["1"].stress.toFixed(2) : "") + "</td");
        if (entry["1"]) {
            $("tr.individual").append("<td><canvas id='" + entry["1"].chart_id + "' /></td>");
            draw_on(entry["1"].chart_id);
        }
        else {
            $("tr.individual").append("<td></td>");
        }
        $("tr.head_incremental").append("<td>" + (entry.i ? entry.i.stress.toFixed(2) : "") + "</td");
        $("tr.head_scratch").append("<td>" + (entry.s ? entry.s.stress.toFixed(2) : "") + "</td");
    });

    const elapsed = new Date() - start;
    console.log("wide page drawing time: " + elapsed + "ms -> " + (1000 / elapsed).toFixed(1) + "fps");
}

function draw_on(chart_id) {
    let settings = {canvas: {width: 300, height: 300}, point_scale: 1};
    $.getJSON(chart_id + ".map.json", function(map_data) {
        draw($("#" + chart_id), [map_data, settings]);
    });
}
