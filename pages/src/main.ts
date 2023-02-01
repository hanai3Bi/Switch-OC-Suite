/* Config: Cust */
const CUST_REV = 3;
var buffer: ArrayBuffer;

enum CustPlatform {
  Undefined = 0,
  Erista = 1,
  Mariko = 2,
  All = 3,
};

class CustEntry {
  id: string;
  name: string;
  platform: CustPlatform;
  size: number;
  desc: string;
  defval: number;
  min: number;
  max: number;
  step: number; // also as quotient
  zeroable: boolean;
  value?: number;
  offset?: number;

  constructor(id: string, name: string, platform: CustPlatform, size: number, desc: string, defval: number, minmax: [number, number] = [0, 1_000_000], step: number = 1, zeroable: boolean = true) {
    this.id = id;
    this.name = name;
    this.platform = platform;
    this.size = size;
    this.desc = desc;
    this.defval = defval;
    this.min = minmax[0];
    this.max = minmax[1];
    this.step = step;
    this.zeroable = zeroable;
  };

  validate(): boolean {
    let tip = new ErrorToolTip(this.id);
    tip.clear();
    if (Number.isNaN(this.value) || this.value === undefined) {
      tip.setMsg(`Invalid value: Not a number`);
      tip.show();
      return false;
    }
    if (this.zeroable && this.value == 0)
      return true;
    if (this.value < this.min || this.value > this.max) {
      tip.setMsg(`Expected range: [${this.min}, ${this.max}], got ${this.value}.`);
      tip.show();
      return false;
    }
    if (this.value % this.step != 0) {
      tip.setMsg(`${this.value} % ${this.step} ≠ 0`);
      tip.show();
      return false;
    }
    return true;
  };

  getInputElement(): HTMLInputElement | null {
    return document.getElementById(this.id) as HTMLInputElement;
  }

  updateValueFromElement() {
    this.value = Number(this.getInputElement()!.value);
  }

  isAvailableFor(platform: CustPlatform): boolean {
    return platform === CustPlatform.Undefined || this.platform === platform || this.platform === CustPlatform.All;
  }

  createElement() {
    let input = this.getInputElement();
    if (!input) {
      let grid = document.createElement("div");
      grid.classList.add("grid", "cust-element");

      // Label and input
      input = document.createElement("input");
      input.min = String(this.zeroable ? 0 : this.min);
      input.max = String(this.max);
      input.id = this.id;
      input.type = "number";
      input.step = String(this.step);
      let label = document.createElement("label");
      label.setAttribute("for", this.id);
      label.innerHTML = this.name;
      label.appendChild(input);
      grid.appendChild(label);

      // Description in blockquote style
      let desc = document.createElement("blockquote");
      desc.innerHTML = this.desc;
      desc.setAttribute("for", this.id);
      grid.appendChild(desc);

      document.getElementById("form")!.appendChild(grid);

      let tooltip = new ErrorToolTip(this.id);
      tooltip.addChangeListener();
    }
    input.value = String(this.value);
  }

  setElementDefaultValue() {
    (document.getElementById(this.id) as HTMLInputElement).value = String(this.defval);
  }

  removeElement() {
    let input = this.getInputElement();
    if (input) {
      input.parentElement!.parentElement!.remove();
    }
  }

  showElement() {
    let input = this.getInputElement();
    if (input) {
      input.parentElement!.parentElement!.style.removeProperty("display");
    }
  }

  hideElement() {
    let input = this.getInputElement();
    if (input) {
      input.parentElement!.parentElement!.style.setProperty("display", "none");
    }
  }
}

var CustTable: Array<CustEntry> = [
  new CustEntry(
    "mtcConf",
    "DRAM Timing",
    CustPlatform.Mariko,
    2,
    "<li><b>0</b>: AUTO_ADJ_MARIKO_SAFE: Auto adjust timings for LPDDR4 ≤3733 Mbps specs, 8Gb density. (Default)</li>\
     <li><b>1</b>: AUTO_ADJ_MARIKO_4266: Auto adjust timings for LPDDR4X 4266 Mbps specs, 8Gb density.</li>\
     <li><b>2</b>: NO_ADJ_ALL: No timing adjustment for both Erista and Mariko. Might achieve better performance on Mariko but lower maximum frequency is expected.",
    0,
    [0, 2],
    1
  ),
  new CustEntry(
    "marikoCpuMaxClock",
    "Mariko CPU Max Clock in kHz",
    CustPlatform.Mariko,
    4,
    "<li>System default: 1785000</li>\
     <li>2397000 might be unreachable for some SoCs.</li>",
    2397_000,
    [1785_000, 3000_000],
    1,
  ),
  new CustEntry(
    "marikoCpuBoostClock",
    "Mariko CPU Boost Clock in kHz",
    CustPlatform.Mariko,
    4,
    "<li>System default: 1785000</li>\
     <li>Boost clock will be applied when applications request higher CPU frequency for quicker loading.</li>\
     <li>This will be set regardless of whether sys-clk is enabled.</li>",
    1785_000,
    [1020_000, 3000_000],
    1,
    false
  ),
  new CustEntry(
    "marikoCpuMaxVolt",
    "Mariko CPU Max Voltage in mV",
    CustPlatform.Mariko,
    4,
    "<li>System default: 1120</li>\
     <li>Acceptable range: 1100 ≤ x ≤ 1300</li>",
    1235,
    [1100, 1300],
    5
  ),
  new CustEntry(
    "marikoGpuMaxClock",
    "Mariko GPU Max Clock in kHz",
    CustPlatform.Mariko,
    4,
    "<li>System default: 921600</li>\
     <li>Tegra X1+ official maximum: 1267200</li>\
     <li>1305600 might be unreachable for some SoCs.</li>",
    1305_600,
    [768_000, 1536_000],
    100,
  ),
  new CustEntry(
    "marikoEmcMaxClock",
    "Mariko RAM Max Clock in kHz",
    CustPlatform.Mariko,
    4,
    "<li>Values should be ≥ 1600000, and divided evenly by 3200.</li>\
     <li><b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM</li>",
    1996_800,
    [1600_000, 2400_000],
    3200,
  ),
  new CustEntry(
    "marikoEmcVddqVolt",
    "EMC Vddq (Mariko Only) Voltage in uV",
    CustPlatform.Mariko,
    4,
    "<li>Acceptable range: 550000 ≤ x ≤ 650000</li>\
     <li>Value should be divided evenly by 5000</li>\
     <li>Default: 600000</li>\
     <li>Not enabled by default.</li>\
     <li>This will not work without sys-clk-OC.</li>",
    0,
    [550_000, 650_000],
    5000,
  ),
  new CustEntry(
    "eristaCpuMaxVolt",
    "Erista CPU Max Voltage in mV",
    CustPlatform.Erista,
    4,
    "<li>Acceptable range: 1100 ≤ x ≤ 1300</li>",
    1235,
    [1100, 1300],
    1,
  ),
  new CustEntry(
    "eristaEmcMaxClock",
    "Erista RAM Max Clock in kHz",
    CustPlatform.Erista,
    4,
    "<li>Values should be ≥ 1600000, and divided evenly by 3200.</li>\
     <li><b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM</li>",
    1862_400,
    [1600_000, 2400_000],
    3200,
  ),
  new CustEntry(
    "commonEmcMemVolt",
    "EMC Vddq (Erista Only) & RAM Vdd2 Voltage in uV",
    CustPlatform.All,
    4,
    "<li>Acceptable range: 1100000 ≤ x ≤ 1250000, and it should be divided evenly by 12500.</li>\
     <li>Erista Default (HOS): 1125000 (bootloader: 1100000)</li>\
     <li>Mariko Default: 1100000 (It will not work without sys-clk-OC)</li>\
     <li>Not enabled by default</li>",
    0,
    [1100_000, 1250_000],
    12500,
  ),
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
  id: string;
  message: string | null;
  element: HTMLElement | null;

  constructor(id: string, msg?: string) {
    this.id = id;
    this.element = document.getElementById(id);
    if (msg) { this.setMsg(msg); }
  };

  setMsg(msg: string) {
    this.message = msg;
  }

  show() {
    if (this.element) {
      this.element.setAttribute("aria-invalid", "true");
      if (this.message) {
        this.element.setAttribute("title", this.message);
        this.element.parentElement!.setAttribute("data-tooltip", this.message);
        this.element.parentElement!.setAttribute("data-placement", "top");
      }
    }
  };

  clear() {
    if (this.element) {
      this.element.removeAttribute("aria-invalid");
      this.element.removeAttribute("title");
      this.element.parentElement!.removeAttribute("data-tooltip");
      this.element.parentElement!.removeAttribute("data-placement");
    }
  }

  addChangeListener() {
    if (this.element) {
      this.element.addEventListener('change', (_evt) => {
        let obj = CustTable.filter((obj) => { return obj.id === this.id; })[0];
        obj.value = Number((this.element as HTMLInputElement).value);
        obj.validate();
      });
    }
  }
};

function SaveCust(buffer) {
  let view = new DataView(buffer);
  let storage = {};

  CustTable.forEach(i => {
    i.updateValueFromElement();
    if (!i.validate()) {
      document.getElementById(i.id)!.focus();
      throw new Error(`Invalid ${i.name}`);
    }
    if (!i.offset) {
      document.getElementById(i.id)!.focus();
      throw new Error(`Failed to get offset for ${i.name}`);;
    }
    switch (i.size) {
      case 2:
        view.setUint16(i.offset, i.value!, true);
        break;
      case 4:
        view.setUint32(i.offset, i.value!, true);
        break;
      default:
        document.getElementById(i.id)!.focus();
        throw new Error(`Unknown size at ${i.name}`);
    }
    storage[i.id] = i.value;
  });

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

function CustNavTabsInit() {
  const custNavTabs = Array.from(document.querySelectorAll(`nav[role="tab-control"] > button`)) as HTMLElement[];
  custNavTabs.forEach(i => {
    i.removeAttribute("disabled");
    let platform = Number(i.getAttribute("data-filter")!) as CustPlatform;
    i.addEventListener('click', (_evt) => {
      const unfocusedClasses = ["outline"];
      i.classList.remove(...unfocusedClasses);
      custNavTabs.filter(j => j != i).forEach(k => k.classList.add(...unfocusedClasses));

      CustTable.forEach(e => {
        if (e.isAvailableFor(platform)) {
          e.showElement();
        } else {
          e.hideElement();
        }
      });
    });
  });
}

function UpdateHTMLForm() {
  CustTable.forEach(i => i.createElement());

  let default_btn = document.getElementById("load_default")!;
  default_btn.removeAttribute("disabled");
  default_btn.addEventListener('click', () => {
    CustTable.forEach(i => i.setElementDefaultValue());
  });

  let last_btn = document.getElementById("load_saved")!;
  if (LastSaved()) {
    last_btn.style.removeProperty("display");
    last_btn.removeAttribute("disabled");
    last_btn.addEventListener('click', () => {
      // Load last saved from localStorage
      JSON.parse(localStorage.getItem("last_saved")!)
        .filter((key: string) => key != "custRev")
        .forEach((key: string) => (document.getElementById(key) as HTMLInputElement).value = key);
    });
  } else {
    last_btn.style.setProperty("display", "none");
  }

  let save_btn = document.getElementById("save")!;
  save_btn.removeAttribute("disabled");
  save_btn.addEventListener('click', () => {
    try {
      SaveCust(buffer);
    } catch (e) {
      console.log(e);
      alert(e);
    }
  });
}

function ParseCust(magicOffset: number, buffer: ArrayBuffer) {
  let view = new DataView(buffer);
  let offset = magicOffset + 4;
  let rev = view.getUint16(offset, true);
  offset += 2;
  if (rev != CUST_REV) {
    throw new Error(`Unsupported custRev, expected: ${CUST_REV}, got ${rev}`);
  }
  document.getElementById("cust_rev")!.innerHTML = `Cust V${CUST_REV} is loaded.`;

  CustTable.forEach(i => {
    i.offset = offset;
    switch (i.size) {
      case 2:
        i.value = view.getUint16(offset, true);
        break;
      case 4:
        i.value = view.getUint32(offset, true);
        break;
      default:
        document.getElementById(i.id)!.focus();
        throw new Error("Unknown size at " + i);
    }
    offset += i.size;
    i.validate();
  });
}

const fileInput = document.getElementById("file") as HTMLInputElement;
fileInput.addEventListener('change', (event) => {
  let reader = new FileReader();
  reader.readAsArrayBuffer((event.target as HTMLInputElement).files![0]);
  reader.onloadend = (progEvent) => {
    if (progEvent.target!.readyState === FileReader.DONE) {
      buffer = (progEvent.target!.result!) as ArrayBuffer;
      try {
        let offset = FindMagicOffset(buffer);
        CustTable.forEach(i => i.removeElement());
        ParseCust(offset, buffer);
        CustNavTabsInit();
        UpdateHTMLForm();
      } catch (e) {
        console.log(e);
        alert(e);
        fileInput.value = "";
      }
    }
  }
});

/* GitHub Release fetch */
type ReleaseInfo = {
  OCSuiteVer: string;
  LoaderKipUrl: string;
  LoaderKipTime: string;
  SdOutZipUrl: string;
  SdOutZipTime: string;
  AMSVer: string;
  AMSUrl: string;
};

async function fetchRelease(): Promise<ReleaseInfo | void> {
  try {
    const responseFromSuite = await fetch('https://api.github.com/repos/KazushiMe/Switch-OC-Suite/releases/latest', {
      method: 'GET',
      headers: {
        Accept: 'application/json',
      },
    });
    if (!responseFromSuite.ok) {
      throw new Error(`Failed to fetch latest release info from GitHub: ${responseFromSuite.status}`);
    }

    const resultFromSuite = await responseFromSuite.json();
    const latestVerFromSuite = resultFromSuite.tag_name;
    const amsVer = latestVerFromSuite.split(".").slice(0, 3).join(".");

    const loaderKip = resultFromSuite.assets.filter((obj) => {
      return obj.name.endsWith("loader.kip");
    })[0];
    const sdOut = resultFromSuite.assets.filter((obj) => {
      return obj.name.endsWith(".zip");
    })[0];
    const amsReleaseUrl = `https://github.com/Atmosphere-NX/Atmosphere/releases/tags/${amsVer}`;

    let info: ReleaseInfo = {
      OCSuiteVer: latestVerFromSuite,
      LoaderKipUrl: loaderKip.browser_download_url,
      LoaderKipTime: loaderKip.updated_at,
      SdOutZipUrl: sdOut.browser_download_url,
      SdOutZipTime: sdOut.updated_at,
      AMSVer: amsVer,
      AMSUrl: amsReleaseUrl
    };
    return info;
  } catch (e) {
    console.log(e);
    alert(e);
  }
}

const isElementVisible = (element: HTMLElement): boolean => {
  let rect = element.getBoundingClientRect();
  return (
    rect.top > 0 &&
    rect.left > 0 &&
    rect.bottom - rect.height < (window.innerHeight || document.documentElement.clientHeight) &&
    rect.right - rect.width < (window.innerWidth || document.documentElement.clientWidth)
  );
};

async function updateDownloadUrls() {
  // Wait until download buttons are visible
  while (!isElementVisible(document.getElementById("download_btn_grid")!)) {
    await new Promise(r => setTimeout(r, 1000));
  };

  const updateHref = (id: string, name: string, url: string) => {
    let element = document.getElementById(id)!;
    element.innerHTML = name;
    element.removeAttribute("aria-busy");
    element.setAttribute("href", url);
  };

  let info = await fetchRelease();
  if (info) {
    const loaderKipName = `loader.kip <b>${info.OCSuiteVer}</b><br>${info.LoaderKipTime}`;
    updateHref("loader_kip_btn", loaderKipName, info.LoaderKipUrl);

    const sdOutName = `SdOut.zip <b>${info.OCSuiteVer}</b><br>${info.SdOutZipTime}`;
    updateHref("sdout_zip_btn", sdOutName, info.SdOutZipUrl);

    const amsName = `Atmosphere-NX <b>${info.AMSVer}</b>`;
    updateHref("ams_btn", amsName, info.AMSUrl);
  }
}

addEventListener('DOMContentLoaded', async (_evt) => {
  await updateDownloadUrls();
});