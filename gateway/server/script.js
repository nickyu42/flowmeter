const plotParent = document.getElementById("24-hours-plot");
const totalConsumptionCounter = document.getElementById("total-consumption-counter");
const currentFlowrateCounter = document.getElementById("current-flowrate-counter");

function getSize() {
    return {
        height: 300,
        width: plotParent.getBoundingClientRect().width
    }
}

function createPlot() {
    const opts = {
        title: "",
        id: "acc-chart",
        ...getSize(),
        series: [
            {},
            {
                label: "Flowrate",
                stroke: "green",
                // width: 1,
                value: (u, v) => v + " (lpm)",
                paths: uPlot.paths.spline(),
            },
            // {
            //     label: "blue",
            //     stroke: "blue",
            //     // width: 1,
            //     paths: uPlot.paths.spline(),
            //     fill: "blue"
            // },
        ],
    };

    // let data2 = [
    //     [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100],
    //     [109, 117, 122, 104, 105, 117, 119, 121, 117, 121, 122, 129, 119, 113, 113, 121, 108, 108, 100, 103, 113, 110, 107, 105, 99, 93, 87, 83, 91, 85, 81, 69, 76, 61, 63, 74, 76, 68, 55, 61, 48, 39, 54, 44, 37, 30, 22, 33, 29, 21, 22, 43, 47, 33, 47, 28, 29, 31, 32, 35, 37, 25, -5, -14, -7, -14, -7, -18, -18, -18, -16, -41, -22, -30, -27, -30, -47, -49, -47, -42, -55, -34, -27, -22, -23, -34, -23, -32, -36, -47, -33, -32, -18, -23, -21, -33, -39, -21, -18, -27, -5],
    // ];

    return new uPlot(opts, [[Date.now() / 1000], [0]], document.getElementById('24-hours-plot'));
}

const dataBuffer = [[], []];

// const HOST = 'portainer.njkyu.com';
// const PORT = 43000;

const HOST = 'localhost';
const PORT = 9001;

const plot = createPlot();
window.addEventListener("resize", e => {
    plot.setSize(getSize());
});

let backoff = 0;

function onMessage(payload) {
    const encodedString = String.fromCharCode.apply(null, payload);
    const json = JSON.parse(encodedString);

    if ("flow_rate" in json) {
        currentFlowrateCounter.textContent = Number(json["flow_rate"]).toFixed(2);

        if (backoff > 5) {
            dataBuffer[0].push(Date.now() / 1000);
            dataBuffer[1].push(Number(json["flow_rate"]));
            backoff = 0;
        } else {
            backoff++;
        }

        plot.setData(dataBuffer);
    }

    if ("cumulative_flow" in json) {
        totalConsumptionCounter.textContent = Number(json["cumulative_flow"]).toFixed(2);
    }
}

function connectToBroker() {
    console.log('Connecting to broker');
    const client = mqtt.connect(`ws://${HOST}:${PORT}`);

    client.subscribe('/flow');

    client.on('message', function (topic, payload) {
        onMessage(payload);
    });

    const statusEl = document.getElementById('status');

    client.on('connect', () => {
        console.log('Connected to server');
        statusEl.style.color = 'green';
        statusEl.innerText = 'connected';
    });

    client.on('close', () => {
        statusEl.style.color = 'red';
        statusEl.innerText = 'offline';
    });

    client.on('error', (err) => {
        console.log(err)
        statusEl.style.color = 'red';
        statusEl.innerText = `error (${err})`;
    });
}

connectToBroker();