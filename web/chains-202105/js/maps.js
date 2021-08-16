//

// ----------------------------------------------------------------------

export const IMAGE_SIZE = 800;

// ----------------------------------------------------------------------

export function make_request_data(data) {
    return Object.entries(data).map((en) => `${en[0]}=${encodeURIComponent(en[1])}`).join('&')
}

// ----------------------------------------------------------------------

export function make_link(data) {
    return `/eu/results/chains-202105/${data.ace}?acv=html`;
}

// ----------------------------------------------------------------------

// returns {title: "text", td: "text"}
export function map_td_with_title(data, merge_type, coloring, save_chart) {
    const result = {};
    if (data[merge_type] && data[merge_type].ace) {
        const req = make_request_data({type: "map", ace: data[merge_type].ace, coloring: coloring, size: IMAGE_SIZE, save_chart: save_chart ? 1 : 0}); // save_chart must be number to properly convert to bool!
        const link = make_link({ace: data[merge_type].ace});
        result.title = `<td>${merge_type}<span class="map-title-space"></span><a href="ace?ace=${data[merge_type].ace}" download>ace</a> <a href="pdf?${req}" download>pdf</a><span class="map-title-space"></span>coloring:${coloring}</td>`;
        result.td = `<td><a href="${link}" target="_blank"><img src="png?${req}"></a></td>`;
    }
    return result.td ? result : null;
}

// ----------------------------------------------------------------------
