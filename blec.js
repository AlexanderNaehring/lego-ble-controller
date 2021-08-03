//BLE
const serviceUUID = 0xffe0; //0000ffe0-0000-1000-8000-00805f9b34fb HM-10 service UUID
const characteristicUUID = 0xffe1; //0000ffe1-0000-1000-8000-00805f9b34fb HM-10 characteristic UUID
let device;
let server;
let characteristic;

//window.onload = () => { }
document.addEventListener('DOMContentLoaded', function () {
    const connectBtn = document.getElementById('connect');

    if (navigator.bluetooth) {
        connectBtn.removeAttribute("disabled");
    } else {
        alert('Your current browser does not support web bluetooth or it is not enabled. \n' +
            'Please use the latest version of Chrome and enable Web Bluetooth under chrome://flags');
    }
    connectBtn.onclick = connectBLE;

    let elements;

    elements = document.getElementsByTagName("input");
    for (let i = 0; i < elements.length; i++) {
        elements[i].addEventListener("touchend", function (e) {
            this.value = "0";
        });
        elements[i].addEventListener("mouseup", function (e) {
            this.value = "0";
        });
    }


    elements = document.querySelectorAll("[data-center]");
    for (let i = 0; i < elements.length; i++) {
        elements[i].addEventListener("input", function (e) {
            // val changed) ...
        });
    }

    setInterval(updateSpeed, 50);
}, false);


async function connectBLE() {
    //BLE setup. Connect and get service/characteristic notifications
    // https://developer.mozilla.org/en-US/docs/Web/API/Bluetooth/requestDevice
    try {
        device = await navigator.bluetooth.requestDevice({filters: [{services: [serviceUUID]}]});  // { filters: [{ services: [service] }] } // {acceptAllDevices: true, optionalServices: [service]}
        server = await device.gatt.connect();
        if (server) {
            const btn = document.getElementById('connect');
            btn.innerText = "Disconnect";
            btn.onclick = disconnectBLE;
        }
        const service = await server.getPrimaryService(serviceUUID);
        characteristic = await service.getCharacteristic(characteristicUUID);
    } catch (e) {
        console.log(e);
        alert(e);
    }
}

function disconnectBLE() {
    const btn = document.getElementById('connect');
    btn.innerText = "Connect";
    btn.onclick = connectBLE;
    if (server) {
        server.disconnect();
    }
    characteristic = undefined;
    device = undefined;
}

function str2ab(str) {
    let buf = new ArrayBuffer(str.length * 2); // 2 bytes for each char
    let bufView = new Uint8Array(buf); //make sure buffer array is of type uint8
    for (let i = 0, strLen = str.length; i < strLen; i++) {
        bufView[i] = str.charCodeAt(i);
    }
    return buf;
}

function updateSpeed() {
    if (server && server.connected && characteristic) {
        const buf = new ArrayBuffer(2);
        const bufView = new Uint8Array(buf);
        let str = "";
        for (let i = 0; i < 2; i++) {
            const val = document.getElementById("m" + (i + 1)).value;
            bufView[i] = val;
            str += val;
        }

        const now = new Date();
        if (this.lastData === undefined)
            this.lastData = null;
        if (this.lastDate === undefined)
            this.lastDate = now;

        if (this.lastData !== str || now - this.lastDate > 400) {
            this.lastData = str;
            this.lastDate = now;
            console.log(str);
            characteristic.writeValueWithoutResponse(buf);
        }
    }
}

// https://stackoverflow.com/questions/21553528/how-to-test-for-equality-in-arraybuffer-dataview-and-typedarray
// compare ArrayBuffers
function arrayBuffersAreEqual(a, b) {
    return dataViewsAreEqual(new DataView(a), new DataView(b));
}

// compare DataViews
function dataViewsAreEqual(a, b) {
    if (a.byteLength !== b.byteLength) return false;
    for (let i=0; i < a.byteLength; i++) {
        if (a.getUint8(i) !== b.getUint8(i)) return false;
    }
    return true;
}

// compare TypedArrays
function typedArraysAreEqual(a, b) {
    if (a.byteLength !== b.byteLength) return false;
    return a.every((val, i) => val === b[i]);
}

