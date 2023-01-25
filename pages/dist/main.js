var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
/* Config: Cust */
var CUST_REV;
var buffer;
class CustEntry {
    constructor(name, size, desc, defval, minmax = [0, 1000000], step = 1, extra_validator) {
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
    ;
}
var CustTable = null;
const CustTableV2 = [
    new CustEntry("mtcConf", 2, "<b>DRAM Timing</b>\
       <li><b>0</b>: AUTO_ADJ_MARIKO_SAFE: Auto adjust timings for LPDDR4 ≤3733 Mbps specs, 8Gb density. (Default)</li>\
       <li><b>1</b>: AUTO_ADJ_MARIKO_4266: Auto adjust timings for LPDDR4X 4266 Mbps specs, 8Gb density.</li>", 0, [0, 3]),
    new CustEntry("marikoCpuMaxClock", 4, "<b>Mariko CPU Max Clock in kHz</b>\
       <li>System default: 1785000</li>\
       <li>2397000 might be unreachable for some SoCs.</li>", 2397000, [1785000, 3000000], 100, (x) => (x % 100) == 0),
    new CustEntry("marikoCpuBoostClock", 4, "<b>Mariko CPU Boost Clock in kHz</b>\
       <li>System default: 1785000</li>\
       <li>Boost clock will be applied when applications request higher CPU frequency for quicker loading.</li>\
       <li>This will be set regardless of whether sys-clk is enabled.</li>", 1785000, [1785000, 3000000], 100, (x) => (x % 100) == 0),
    new CustEntry("marikoCpuMaxVolt", 4, "<b>Mariko CPU Max Voltage in mV</b>\
       <li>System default: 1120</li>\
       <li>Acceptable range: 1100 ≤ x ≤ 1300</li>", 1235, [1100, 1300]),
    new CustEntry("marikoGpuMaxClock", 4, "<b>Mariko GPU Max Clock in kHz</b>\
       <li>System default: 921600</li>\
       <li>Tegra X1+ official maximum: 1267200</li>\
       <li>1305600 might be unreachable for some SoCs.</li>", 1305600, [768000, 1536000], 100, (x) => (x % 100) == 0),
    new CustEntry("marikoEmcMaxClock", 4, "<b>Mariko RAM Max Clock in kHz</b>\
       <li>Values should be > 1600000, and divided evenly by 3200.</li>\
       <li><b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM</li>", 1996800, [1612800, 2400000], 3200, (x) => (x % 3200) == 0),
    new CustEntry("eristaOCEnable", 4, "<b>Erista CPU Enable Overclock</b>\
    <li>Not tested</li>", 1, [0, 1]),
    new CustEntry("eristaCpuMaxVolt", 4, "<b>Erista CPU Max Voltage in mV</b>\
       <li>Acceptable range: 1100 ≤ x ≤ 1300</li>", 1235, [0, 1300], 1, (x) => x >= 1100),
    new CustEntry("eristaEmcMaxClock", 4, "<b>Erista RAM Max Clock in kHz</b>\
       <li>Values should be > 1600000, and divided evenly by 3200.</li>\
       <li><b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM</li>", 1862400, [1600000, 2400000], 3200, (x) => (x % 3200) == 0),
    new CustEntry("eristaEmcVolt", 4, "<b>Erista RAM Voltage in uV</b>\
       <li>Acceptable range: 1100000 ≤ x ≤ 1250000, and it should be divided evenly by 12500.</li>\
       <li>Not enabled by default</li>", 0, [0, 1250000], 12500, (x) => (x % 12500) == 0 && x >= 1100000),
];
const CustTableV3 = [
    new CustEntry("mtcConf", 2, "<b>DRAM Timing</b>\
       <li><b>0</b>: AUTO_ADJ_MARIKO_SAFE: Auto adjust timings for LPDDR4 ≤3733 Mbps specs, 8Gb density. (Default)</li>\
       <li><b>1</b>: AUTO_ADJ_MARIKO_4266: Auto adjust timings for LPDDR4X 4266 Mbps specs, 8Gb density.</li>\
       <li><b>2</b>: NO_ADJ_ALL: No timing adjustment for both Erista and Mariko. Might achieve better performance on Mariko but lower maximum frequency is expected.", 0, [0, 3]),
    new CustEntry("marikoCpuMaxClock", 4, "<b>Mariko CPU Max Clock in kHz</b>\
       <li>System default: 1785000</li>\
       <li>2397000 might be unreachable for some SoCs.</li>", 2397000, [1785000, 3000000], 100, (x) => (x % 100) == 0),
    new CustEntry("marikoCpuBoostClock", 4, "<b>Mariko CPU Boost Clock in kHz</b>\
       <li>System default: 1785000</li>\
       <li>Boost clock will be applied when applications request higher CPU frequency for quicker loading.</li>\
       <li>This will be set regardless of whether sys-clk is enabled.</li>", 1785000, [1785000, 3000000], 100, (x) => (x % 100) == 0),
    new CustEntry("marikoCpuMaxVolt", 4, "<b>Mariko CPU Max Voltage in mV</b>\
       <li>System default: 1120</li>\
       <li>Acceptable range: 1100 ≤ x ≤ 1300</li>", 1235, [1100, 1300]),
    new CustEntry("marikoGpuMaxClock", 4, "<b>Mariko GPU Max Clock in kHz</b>\
       <li>System default: 921600</li>\
       <li>Tegra X1+ official maximum: 1267200</li>\
       <li>1305600 might be unreachable for some SoCs.</li>", 1305600, [768000, 1536000], 100, (x) => (x % 100) == 0),
    new CustEntry("marikoEmcMaxClock", 4, "<b>Mariko RAM Max Clock in kHz</b>\
       <li>Values should be > 1600000, and divided evenly by 3200.</li>\
       <li><b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM</li>", 1996800, [1612800, 2400000], 3200, (x) => (x % 3200) == 0),
    new CustEntry("marikoEmcVolt", 4, "<b>Mariko RAM Voltage in uV</b>\
      <li>Acceptable range: 600000 ≤ x ≤ 650000</li>\
      <li>Value should be divided evenly by 5'000</li>\
      <li>Not enabled by default.</li>\
      <li>This will not work without sys-clk-OC.</li>", 1, [0, 1]),
    new CustEntry("eristaCpuMaxVolt", 4, "<b>Erista CPU Max Voltage in mV</b>\
       <li>Acceptable range: 1100 ≤ x ≤ 1300</li>", 1235, [0, 1300], 1, (x) => x >= 1100),
    new CustEntry("eristaEmcMaxClock", 4, "<b>Erista RAM Max Clock in kHz</b>\
       <li>Values should be > 1600000, and divided evenly by 3200.</li>\
       <li><b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM</li>", 1862400, [1600000, 2400000], 3200, (x) => (x % 3200) == 0),
    new CustEntry("eristaEmcVolt", 4, "<b>Erista RAM Voltage in uV</b>\
       <li>Acceptable range: 1100000 ≤ x ≤ 1250000, and it should be divided evenly by 12500.</li>\
       <li>Not enabled by default</li>", 0, [0, 1250000], 12500, (x) => (x % 12500) == 0 && x >= 1100000),
];
function FindMagicOffset(buffer) {
    let view = new DataView(buffer);
    for (let i = 0; i < view.byteLength; i += 4) {
        if (view.getUint32(i, true) == 0x54535543) { // "CUST"
            return i;
        }
    }
    throw new Error("Invalid loader.kip file");
}
class ErrorToolTip {
    constructor(id) {
        this.id = id;
        this.element = document.getElementById(id);
    }
    ;
    setMsg(msg) {
        this.message = msg;
    }
    show() {
        if (this.element) {
            this.element.setAttribute("aria-invalid", "true");
            if (this.message) {
                this.element.setAttribute("title", this.message);
                this.element.parentElement.setAttribute("data-tooltip", this.message);
                this.element.parentElement.setAttribute("data-placement", "top");
            }
        }
    }
    ;
    clear() {
        if (this.element) {
            this.element.removeAttribute("aria-invalid");
            this.element.removeAttribute("title");
            this.element.parentElement.removeAttribute("data-tooltip");
            this.element.parentElement.removeAttribute("data-placement");
        }
    }
    addChangeListener() {
        if (this.element) {
            this.element.addEventListener('change', (_evt) => {
                let obj = CustTable.filter((obj) => { return obj.name === this.id; })[0];
                obj.value = Number(this.element.value);
                ValidateCustEntry(obj);
            });
        }
    }
}
;
function ValidateCustEntry(entry) {
    let elementId = entry.name;
    let tip = new ErrorToolTip(elementId);
    tip.clear();
    if (entry.value == 0)
        return null;
    if (entry.value < entry.min || entry.value > entry.max) {
        tip.setMsg(`Expected range: [${entry.min}, ${entry.max}], got ${entry.value}.`);
        tip.show();
        return tip;
    }
    if (entry.validator && !entry.validator(entry.value)) {
        tip.setMsg(`Invalid value: ${entry.value}. Did not pass this validator: ${entry.validator}.`);
        tip.show();
        return tip;
    }
    return null;
}
function ValidateCust() {
    let tooltips = [];
    for (let i of CustTable) {
        let tip = ValidateCustEntry(i);
        if (tip) {
            tooltips.push(tip);
        }
    }
    if (tooltips.length > 0) {
        throw new Error("Invalid cust");
    }
}
function SaveCust(buffer) {
    let view = new DataView(buffer);
    let storage = {};
    storage["custRev"] = CUST_REV;
    for (let i of CustTable) {
        let id = i.name;
        i.value = Number(document.getElementById(id).value);
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
    ValidateCust();
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
function LoadDefault() {
    for (let i of CustTable) {
        let id = i.name;
        document.getElementById(id).value = String(i.defval);
    }
}
function ClearHTMLForm() {
    var _a, _b;
    if (!CustTable) {
        return;
    }
    for (let i of CustTable) {
        let id = i.name;
        let input = document.getElementById(id);
        (_b = (_a = input.parentElement) === null || _a === void 0 ? void 0 : _a.parentElement) === null || _b === void 0 ? void 0 : _b.remove();
    }
}
function UpdateHTMLForm() {
    let dict = Object.assign({}, ...CustTable.map((x) => ({ [x.name]: x })));
    let form = document.getElementById("form");
    for (let i of CustTable) {
        let id = i.name;
        let input = document.getElementById(id);
        if (!input) {
            let grid = document.createElement("div");
            grid.classList.add("grid");
            // Label and input box
            input = document.createElement("input");
            input.min = dict[i.name].min;
            input.max = dict[i.name].max;
            input.id = id;
            input.type = "number";
            input.step = dict[i.name].step;
            let label = document.createElement("label");
            label.setAttribute("for", id);
            label.innerHTML = id;
            label.appendChild(input);
            grid.appendChild(label);
            // Description in blockquote style
            let desc = dict[i.name].desc;
            let block = document.createElement("blockquote");
            block.style["margin-top"] = "0";
            block.innerHTML = desc;
            block.setAttribute("for", id);
            grid.appendChild(block);
            grid.style["margin-top"] = "3rem";
            form.appendChild(grid);
            let tooltip = new ErrorToolTip(id);
            tooltip.addChangeListener();
        }
        input.value = dict[i.name].value;
    }
    let default_btn = document.getElementById("load_default");
    default_btn.removeAttribute("disabled");
    default_btn.addEventListener('click', () => {
        LoadDefault();
    });
    if (LastSaved()) {
        let last_btn = document.getElementById("load_saved");
        last_btn.removeAttribute("disabled");
        last_btn.addEventListener('click', () => {
            LoadLastSaved();
        });
    }
    let save_btn = document.getElementById("save");
    save_btn.removeAttribute("disabled");
    save_btn.addEventListener('click', () => {
        try {
            SaveCust(buffer);
        }
        catch (e) {
            console.log(e);
            alert(e);
        }
    });
}
function ParseCust(magicOffset, buffer) {
    let view = new DataView(buffer);
    let offset = magicOffset + 4;
    let rev = view.getUint16(offset, true);
    if (rev != 2 && rev != 3) {
        throw new Error("Unsupported custRev, expected: 2 or 3, got " + rev);
    }
    CUST_REV = rev;
    document.getElementById("cust_rev").innerHTML = `Cust V${CUST_REV} is loaded.`;
    if (rev == 2) {
        CustTable = CustTableV2;
    }
    else {
        CustTable = CustTableV3;
    }
    offset += 2;
    for (let i of CustTable) {
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
}
const fileInput = document.getElementById("file");
fileInput.addEventListener('change', (event) => {
    let reader = new FileReader();
    reader.readAsArrayBuffer(event.target.files[0]);
    reader.onloadend = (progEvent) => {
        if (progEvent.target.readyState === FileReader.DONE) {
            buffer = progEvent.target.result;
            try {
                let offset = FindMagicOffset(buffer);
                ClearHTMLForm();
                ParseCust(offset, buffer);
                ValidateCust();
                UpdateHTMLForm();
            }
            catch (e) {
                console.log(e);
                alert(e);
                fileInput.value = "";
            }
        }
    };
});
function fetchRelease() {
    return __awaiter(this, void 0, void 0, function* () {
        try {
            const responseFromSuite = yield fetch('https://api.github.com/repos/KazushiMe/Switch-OC-Suite/releases/latest', {
                method: 'GET',
                headers: {
                    Accept: 'application/json',
                },
            });
            if (!responseFromSuite.ok) {
                throw new Error(`Failed to fetch latest release info from GitHub: ${responseFromSuite.status}`);
            }
            const resultFromSuite = yield responseFromSuite.json();
            const latestVerFromSuite = resultFromSuite.tag_name;
            const correspondingVerFromAMS = latestVerFromSuite.split(".").slice(0, 3).join(".");
            const loaderKip = resultFromSuite.assets.filter((obj) => {
                return obj.name.endsWith("loader.kip");
            })[0];
            const sdOut = resultFromSuite.assets.filter((obj) => {
                return obj.name.endsWith(".zip");
            })[0];
            const amsReleaseUrl = `https://github.com/Atmosphere-NX/Atmosphere/releases/tags/${correspondingVerFromAMS}`;
            let info = {
                OCSuiteVer: latestVerFromSuite,
                LoaderKipUrl: loaderKip.browser_download_url,
                SdOutZipUrl: sdOut.browser_download_url,
                AMSVer: correspondingVerFromAMS,
                AMSUrl: amsReleaseUrl
            };
            return info;
        }
        catch (e) {
            console.log(e);
            alert(e);
        }
    });
}
function updateDownloadUrls() {
    return __awaiter(this, void 0, void 0, function* () {
        const updateHref = (id, name, url) => {
            let element = document.getElementById(id);
            element.innerHTML = name;
            element.removeAttribute("aria-busy");
            element.setAttribute("href", url);
        };
        let info = yield fetchRelease();
        if (info) {
            const loaderKipName = `loader.kip ${info.OCSuiteVer}`;
            updateHref("loader_kip_btn", loaderKipName, info.LoaderKipUrl);
            const sdOutName = `SdOut.zip ${info.OCSuiteVer}`;
            updateHref("sdout_zip_btn", sdOutName, info.SdOutZipUrl);
            const amsName = `Atmosphere-NX ${info.AMSVer}`;
            updateHref("ams_btn", amsName, info.AMSUrl);
        }
    });
}
addEventListener('DOMContentLoaded', (_evt) => __awaiter(this, void 0, void 0, function* () {
    yield updateDownloadUrls();
}));
