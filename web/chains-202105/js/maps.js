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
