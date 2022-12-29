const CUST_REV = 2;
var buffer;
function FindMagicOffset(buffer) {
    let view = new DataView(buffer);
    for (let i = 0; i < view.byteLength; i += 4) {
        if (view.getUint32(i, true) == 0x54535543) { // "CUST"
            return i;
        }
    }
    throw new Error("Invalid loader.kip file");
}
function CustEntry(name, size, desc, defval, minmax = [0, 1000000], step = 1, extra_validator) {
    this.name = name;
    this.size = size;
    this.desc = desc;
    this.defval = defval;
    this.min = minmax[0];
    this.max = minmax[1];
    this.step = step;
    this.validator = extra_validator;
    this.value = null;
    this.offset = null;
}
function InitCustTable() {
    let cust = [
        new CustEntry("mtcConf", 2, "<b>DRAM Timing</b>\
       <li><b>0</b>: AUTO_ADJ_MARIKO_SAFE: Auto adjust timings for LPDDR4 ≤3733 Mbps specs, 8Gb density.</li>\
       <li><b>1</b>: AUTO_ADJ_MARIKO_4266: Auto adjust timings for LPDDR4X 4266 Mbps specs, 8Gb density.</li>", 0, [0, 3]),
        new CustEntry("marikoCpuMaxClock", 4, "<b>Mariko CPU Max Clock in kHz</b>\
       <li>System default: 1785000</li>\
       <li>≥ 2397000 will enable overvolting (> 1120 mV)</li>", 2397000, [1785000, 3000000], 100, (x) => (x % 100) == 0),
        new CustEntry("marikoCpuBoostClock", 4, "<b>Mariko CPU Boost Clock in kHz</b>\
       <li>System default: 1785000</li>\
       <li>Must not be higher than marikoCpuMaxClock</li>", 1785000, [1785000, 3000000], 100, (x) => (x % 100) == 0),
        new CustEntry("marikoCpuMaxVolt", 4, "<b>Mariko CPU Max Voltage in mV</b>\
       <li>System default: 1120</li>\
       <li>Acceptable range: 1100 ≤ x ≤ 1300</li>", 1235, [1100, 1300]),
        new CustEntry("marikoGpuMaxClock", 4, "<b>Mariko GPU Max Clock in kHz</b>\
       <li>System default: 921600</li>\
       <li>Tegra X1+ official maximum: 1267200</li>", 1305600, [768000, 1536000], 100, (x) => (x % 100) == 0),
        new CustEntry("marikoEmcMaxClock", 4, "<b>Mariko RAM Max Clock in kHz</b>\
       <li>Values should be > 1600000, and divided evenly by 3200.</li>\
       <li><b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM</li>", 1996800, [1612800, 2400000], 3200, (x) => (x % 3200) == 0),
        new CustEntry("eristaCpuOCEnable", 4, "<b>Erista CPU Enable Overclock</b>\
       <li>Not tested</li>", 1, [0, 1]),
        new CustEntry("eristaCpuMaxVolt", 4, "<b>Erista CPU Max Voltage in mV</b>\
       <li>Acceptable range: 1100 ≤ x ≤ 1400</li>", 1257, [0, 1400], 100, (x) => x >= 1100),
        new CustEntry("eristaEmcMaxClock", 4, "<b>Erista RAM Max Clock in kHz</b>\
       <li>Values should be > 1600000, and divided evenly by 3200.</li>\
       <li><b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM</li>", 1862400, [1600000, 2400000], 3200, (x) => (x % 3200) == 0),
        new CustEntry("eristaEmcVolt", 4, "<b>Erista RAM Voltage in uV</b>\
       <li>Acceptable range: 1100000 ≤ x ≤ 1250000, and it should be divided evenly by 12500.</li>\
       <li>Not enabled by default</li>", 0, [0, 1250000], 12500, (x) => (x % 12500) == 0 && x >= 1100000),
    ];
    return cust;
}
function ValidateCust(cust) {
    for (let i of cust) {
        if (i.value == 0)
            continue;
        if (i.value < i.min || i.value > i.max) {
            document.getElementById(i.name).focus();
            throw new Error(`Expected range: ${i.min} ≤ ${i.name} ≤ ${i.max}, got ${i.value}`);
        }
        if (i.validator && !i.validator(i.value)) {
            document.getElementById(i.name).focus();
            throw new Error(`Invalid value: ${i.value}(${i.name})\nValidator: ${i.validator}`);
        }
    }
}
function SaveCust(cust, buffer) {
    let dict = Object.assign({}, ...cust.map((x) => ({ [x.name]: x })));
    let view = new DataView(buffer);
    let storage = {};
    for (let i of cust) {
        let id = i.name;
        i.value = document.getElementById(id).value;
        storage[i.name] = i.value;
        switch (i.size) {
            case 2:
                view.setUint16(i.offset, i.value, true);
                break;
            case 4:
                view.setUint32(i.offset, i.value, true);
                break;
            default:
                document.getElementById(i.name).focus();
                throw new Error("Unknown size at " + i);
        }
    }
    ValidateCust(cust);
    storage["custRev"] = CUST_REV;
    localStorage.setItem("last_saved", JSON.stringify(storage));
    let a = document.createElement("a");
    a.href = window.URL.createObjectURL(new Blob([buffer], { type: "application/octet-stream" }));
    a.download = "loader.kip";
    a.click();
}
function LastSaved() {
    let storage = localStorage.getItem("last_saved");
    if (!storage) {
        return false;
    }
    let sObj = JSON.parse(storage);
    if (!sObj["custRev"] || sObj["custRev"] != CUST_REV) {
        localStorage.removeItem("last_saved");
        return false;
    }
    return true;
}
function LoadLastSaved() {
    if (LastSaved()) {
        let storage = localStorage.getItem("last_saved");
        let sObj = JSON.parse(storage);
        for (let key in sObj) {
            if (key == "custRev") {
                continue;
            }
            document.getElementById(key).value = sObj[key];
        }
    }
}
function LoadDefault(cust) {
    let dict = Object.assign({}, ...cust.map((x) => ({ [x.name]: x })));
    for (let i of cust) {
        let id = i.name;
        document.getElementById(id).value = i.defval;
    }
}
function UpdateHTMLForm(cust) {
    let dict = Object.assign({}, ...cust.map((x) => ({ [x.name]: x })));
    let form = document.getElementById("form");
    for (let i of cust) {
        let id = i.name;
        let input = document.getElementById(id);
        if (!input) {
            let div = document.createElement("div");
            div.classList.add("form-floating", "my-3");
            let label = document.createElement("label");
            label.setAttribute("for", id);
            label.innerHTML = id;
            label.classList.add("form-entry");
            input = document.createElement("input");
            input.classList.add("form-control", "form-entry");
            input.min = dict[i.name].min;
            input.max = dict[i.name].max;
            input.id = id;
            input.type = "number";
            input.step = dict[i.name].step;
            div.appendChild(input);
            div.appendChild(label);
            let desc = dict[i.name].desc;
            if (desc) {
                let tip = document.createElement("small");
                tip.innerHTML = desc;
                tip.classList.add("form-entry");
                tip.setAttribute("for", id);
                div.appendChild(tip);
            }
            form === null || form === void 0 ? void 0 : form.appendChild(div);
        }
        input.value = dict[i.name].value;
    }
    let btn = document.getElementById("load");
    btn.classList.remove("hide");
    if (LastSaved()) {
        btn.innerHTML = "Load Last Saved";
        btn.addEventListener('click', () => {
            LoadLastSaved();
        });
    }
    else {
        btn.addEventListener('click', () => {
            LoadDefault(cust);
        });
    }
    btn = document.getElementById("save");
    btn.classList.remove("hide");
    btn.addEventListener('click', () => {
        try {
            SaveCust(cust, buffer);
        }
        catch (e) {
            console.log(e);
            alert(e);
        }
    });
}
function ParseCust(magicOffset, buffer) {
    let view = new DataView(buffer);
    let cust = InitCustTable();
    let offset = magicOffset + 4;
    let rev = view.getUint16(offset, true);
    if (rev != CUST_REV) {
        throw new Error("Unsupported custRev, expected: " + CUST_REV + ", got " + rev);
    }
    offset += 2;
    for (let i of cust) {
        i.offset = offset;
        switch (i.size) {
            case 2:
                i.value = view.getUint16(offset, true);
                break;
            case 4:
                i.value = view.getUint32(offset, true);
                break;
            default:
                document.getElementById(i.name).focus();
                throw new Error("Unknown size at " + i);
        }
        offset += i.size;
    }
    ValidateCust(cust);
    UpdateHTMLForm(cust);
}
const fileInput = document.getElementById("file");
fileInput.addEventListener('change', (event) => {
    let reader = new FileReader();
    reader.readAsArrayBuffer(event.target.files[0]);
    reader.onloadend = (progEvent) => {
        if (progEvent.target.readyState === FileReader.DONE) {
            try {
                buffer = progEvent.target.result;
                let offset = FindMagicOffset(buffer);
                ParseCust(offset, buffer);
            }
            catch (e) {
                console.log(e);
                alert(e);
                fileInput.value = "";
            }
        }
    };
});
