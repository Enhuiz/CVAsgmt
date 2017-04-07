function get_input_app(el, n) {
    let cells = []
    for (let i = 0; i < n; ++i) {
        cells.push({
            id: i,
            img: "",
        });
    };

    return new Vue({
        el: el,

        data: {
            cells: cells,
        },

        methods: {
            // input method
            onAddClicked: function(e) {
                if (e.isTrusted) {
                    e.target.nextElementSibling.click();
                }
            },

            onFileChanged: function(e, cell) {
                let files = e.target.files || e.dataTransfer.files;
                if (files.length) {
                    let image = new Image();
                    let reader = new FileReader();
                    let app = this;

                    reader.onload = (e) => {
                        cell.img = e.target.result;
                    };
                    reader.readAsDataURL(files[0]);
                }
            },

            data: function() {
                let data = {};
                cells.forEach((cell) => {
                    if (cell.img) {
                        // add img# and remove the prefix of urlencoded base64 png
                        data["img#" + cell.id] = cell.img.split(',')[1];
                    }
                });
                return data;
            }
        }
    });
};

function get_output_app(el) {

    return new Vue({
        el: el,

        data: {
            cells: [],
            outsp: []
        },

        methods: {
            clear: function() {
                this.cells = [];
                this.outsp = [];
            },
            // run the processing
            run: function(insp, inip) {
                // merge inip and insp
                let data = {}
                for (let key in inip) {
                    if (inip[key])
                        data[key] = inip[key];
                }
                for (let key in insp) {
                    if (insp[key])
                        data[key] = insp[key];
                }

                // assure cells access
                let self = this;

                fetch(window.location.href, {
                    method: "POST",
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    },
                    body: Object.keys(data).map((key) => {
                        return key + '=' + data[key];
                    }).join('&'),
                }).then(res => {
                    return res.json();
                }).then(data => {
                    // apply them to the cells
                    for (let key in data) {
                        if (key.indexOf("img#") != -1) {
                            self.cells.push({
                                id: key.split("img#").join(''),
                                img: "data:image/png;base64," + data[key]
                            })
                        } else {
                            self.outsp.push({
                                k: key,
                                v: data[key]
                            })
                        }
                    }
                });
            }
        }
    });
};