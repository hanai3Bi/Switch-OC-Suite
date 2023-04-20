/* Config: Cust */
const CUST_REV = 4;

enum CustPlatform {
  Undefined = 0,
  Erista = 1,
  Mariko = 2,
  All = 3,
};

class CustEntry {
  min: number;
  max: number;
  value?: number;
  offset?: number;

  constructor(
    public id: string,
    public name: string,
    public platform: CustPlatform,
    public size: number,
    public desc: string[],
    public defval: number,
    minmax: [number, number] = [0, 1_000_000],
    public step: number = 1,
    public zeroable: boolean = true) {
    this.min = minmax[0];
    this.max = minmax[1];
  };

  validate(): boolean {
    let tip = new ErrorToolTip(this.id).clear();
    if (Number.isNaN(this.value) || this.value === undefined) {
      tip.setMsg(`Invalid value: Not a number`).show();
      return false;
    }
    if (this.zeroable && this.value == 0)
      return true;
    if (this.value < this.min || this.value > this.max) {
      tip.setMsg(`Expected range: [${this.min}, ${this.max}], got ${this.value}.`).show();
      return false;
    }
    if (this.value % this.step != 0) {
      tip.setMsg(`${this.value} % ${this.step} ≠ 0`).show();
      return false;
    }
    return true;
  };

  getInputElement(): HTMLInputElement | null {
    return document.getElementById(this.id) as HTMLInputElement;
  }

  updateValueFromElement() {
    this.value = Number(this.getInputElement()?.value);
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
      desc.innerHTML = "<ul>" + this.desc.map(i => `<li>${i}</li>`).join('') + "</ul>";
      desc.setAttribute("for", this.id);
      grid.appendChild(desc);

      document.getElementById("config-list-basic")!.appendChild(grid);

      new ErrorToolTip(this.id).addChangeListener();
    }
    input.value = String(this.value);
  }

  setElementValue() {
    this.getInputElement()!.value = String(this.value!);
  }

  setElementDefaultValue() {
    this.getInputElement()!.value = String(this.defval);
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
    4,
    ["<b>0</b>: AUTO_ADJ_MARIKO_SAFE: Auto adjust timings for LPDDR4 ≤3733 Mbps specs, 8Gb density. (Default)",
     "<b>1</b>: AUTO_ADJ_MARIKO_4266: Auto adjust timings for LPDDR4X 4266 Mbps specs, 8Gb density.",
     "<b>2</b>: NO_ADJ_ALL: No timing adjustment for both Erista and Mariko. Might achieve better performance on Mariko but lower maximum frequency is expected."],
    0,
    [0, 2],
    1
  ),
  new CustEntry(
    "commonCpuBoostClock",
    "Boost Clock in kHz",
    CustPlatform.All,
    4,
    ["System default: 1785000",
     "Boost clock will be applied when applications request higher CPU frequency for quicker loading.",
     "This will be set regardless of whether sys-clk is enabled."],
    1785_000,
    [1020_000, 3000_000],
    1,
    false
  ),
  new CustEntry(
    "commonEmcMemVolt",
    "EMC Vddq (Erista Only) & RAM Vdd2 Voltage in uV",
    CustPlatform.All,
    4,
    ["Acceptable range: 1100000 ≤ x ≤ 1250000, and it should be divided evenly by 12500.",
     "Erista Default (HOS): 1125000 (bootloader: 1100000)",
     "Mariko Default: 1100000 (It will not work without sys-clk-OC)",
     "Not enabled by default"],
    0,
    [1100_000, 1250_000],
    12500,
  ),
  new CustEntry(
    "eristaCpuMaxVolt",
    "Erista CPU Max Voltage in mV",
    CustPlatform.Erista,
    4,
    ["Acceptable range: 1100 ≤ x ≤ 1300",
     "L4T Default: 1235"],
    1235,
    [1100, 1300],
    1,
  ),
  new CustEntry(
    "eristaEmcMaxClock",
    "Erista RAM Max Clock in kHz",
    CustPlatform.Erista,
    4,
    ["Values should be ≥ 1600000, and divided evenly by 3200.",
     "<b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM"],
    1862_400,
    [1600_000, 2400_000],
    3200,
  ),
  new CustEntry(
    "marikoCpuMaxVolt",
    "Mariko CPU Max Voltage in mV",
    CustPlatform.Mariko,
    4,
    ["System default: 1120",
     "Acceptable range: 1100 ≤ x ≤ 1300"],
    1235,
    [1100, 1300],
    5
  ),
  new CustEntry(
    "marikoEmcMaxClock",
    "Mariko RAM Max Clock in kHz",
    CustPlatform.Mariko,
    4,
    ["Values should be ≥ 1600000, and divided evenly by 3200.",
     "<b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM."],
    1996_800,
    [1600_000, 2400_000],
    3200,
  ),
  new CustEntry(
    "marikoEmcVddqVolt",
    "EMC Vddq (Mariko Only) Voltage in uV",
    CustPlatform.Mariko,
    4,
    ["Acceptable range: 550000 ≤ x ≤ 650000",
     "Value should be divided evenly by 5000",
     "Default: 600000",
     "Not enabled by default.",
     "This will not work without sys-clk-OC."],
    0,
    [550_000, 650_000],
    5000,
  ),
];

class ErrorToolTip {
  element: HTMLElement | null;

  constructor(public id: string, public msg?: string) {
    this.id = id;
    this.element = document.getElementById(id);
    if (msg) { this.setMsg(msg); }
  };

  setMsg(msg: string): ErrorToolTip {
    this.msg = msg;
    return this;
  }

  show(): ErrorToolTip {
    this.element?.setAttribute("aria-invalid", "true");
    if (this.msg) {
      this.element?.setAttribute("title", this.msg);
      this.element?.parentElement?.setAttribute("data-tooltip", this.msg);
      this.element?.parentElement?.setAttribute("data-placement", "top");
    }
    return this;
  };

  clear(): ErrorToolTip {
    this.element?.removeAttribute("aria-invalid");
    this.element?.removeAttribute("title");
    this.element?.parentElement?.removeAttribute("data-tooltip");
    this.element?.parentElement?.removeAttribute("data-placement");
    return this;
  }

  addChangeListener() {
    this.element?.addEventListener('change', (_evt) => {
      let obj = CustTable.filter((obj) => { return obj.id === this.id; })[0];
      obj.value = Number((this.element as HTMLInputElement).value);
      obj.validate();
    });
  }
};

class CustStorage {
  storage: { [key: string]: (number | undefined) } = {};
  readonly key = "last_saved";

  updateFromTable() {
    CustTable.forEach(i => {
      i.updateValueFromElement();
      if (!i.validate()) {
        i.getInputElement()?.focus();
        throw new Error(`Invalid ${i.name}`);
      }
    });

    this.storage = {};
    let kv = Object.fromEntries(CustTable.map((i) => [i.id, i.value]));
    Object.keys(kv)
          .forEach(k => this.storage[k] = kv[k]);
  }

  setTable() {
    let keys = Object.keys(this.storage);
    keys.forEach(k => CustTable.filter(i => i.id == k)[0].value = this.storage[k]);

    // Set default for missing values
    CustTable.filter(i => !keys.includes(i.id))
             .forEach(i => i.value = i.defval);

    CustTable.forEach(i => {
      if (!i.validate()) {
        i.getInputElement()?.focus();
        throw new Error(`Invalid ${i.name}`);
      }
      i.setElementValue();
    });
  }

  save() {
    localStorage.setItem(this.key, JSON.stringify(this.storage));
  }

  load(): { [key: string]: (number | undefined) } | null {
    let s = localStorage.getItem(this.key);
    if (!s) {
      return null;
    }
    let dict = JSON.parse(s);
    let keys = CustTable.map(i => i.id);
    let ignoredKeys: string[] = Object.keys(dict).filter(k => !keys.includes(k));
    if (ignoredKeys.length) {
      console.log(`Ignored: ${ignoredKeys}`);
    }
    Object.keys(dict)
          .filter(k => keys.includes(k))
          .forEach(k => this.storage[k] = dict[k]);
    return this.storage;
  }
}

class Cust {
  buffer: ArrayBuffer;
  view: DataView;
  beginOffset: number;
  storage: CustStorage = new CustStorage();
  readonly magic = 0x54535543; // "CUST"
  readonly magicLen = 4;

  mapper : {[size: number]: { get: any, set: any }} = {
    2: {
      get: (offset: number) => this.view.getUint16(offset, true),
      set: (offset: number, value: number) => this.view.setUint16(offset, value, true),
    },
    4: {
      get: (offset: number) => this.view.getUint32(offset, true),
      set: (offset: number, value: number) => this.view.setUint32(offset, value, true) },
  };

  findMagicOffset() {
    this.view = new DataView(this.buffer);
    for (let offset = 0; offset < this.view.byteLength; offset += this.magicLen) {
      if (this.mapper[this.magicLen].get(offset) == this.magic) {
        this.beginOffset = offset;
        return;
      }
    }
    throw new Error("Invalid loader.kip file");
  }

  save() {
    this.storage.updateFromTable();
    CustTable.forEach(i => {
      if (!i.offset) {
        i.getInputElement()?.focus();
        throw new Error(`Failed to get offset for ${i.name}`);
      }
      let mapper = this.mapper[i.size];
      if (!mapper) {
        i.getInputElement()?.focus();
        throw new Error(`Unknown size at ${i.name}`);
      }
      mapper.set(i.offset, i.value!);
    });
    this.storage.save();

    let a = document.createElement("a");
    a.href = window.URL.createObjectURL(new Blob([this.buffer], { type: "application/octet-stream" }));
    a.download = "loader.kip";
    a.click();

    this.toggleLoadLastSavedBtn(true);
  }

  removeHTMLForm() {
    CustTable.forEach(i => i.removeElement());
  }

  toggleLoadLastSavedBtn(enable: boolean) {
    let last_btn = document.getElementById("load_saved")!;
    if (enable) {
      last_btn.addEventListener('click', () => {
        if (this.storage.load()) {
          this.storage.setTable();
        }
      });
      last_btn.style.removeProperty("display");
      last_btn.removeAttribute("disabled");
    } else {
      last_btn.style.setProperty("display", "none");
    }
  }

  createHTMLForm() {
    CustTable.forEach(i => i.createElement());

    let advanced = document.createElement("p");
    advanced.innerHTML = "Advanced configuration: Coming soon...";
    document.getElementById("config-list-advanced")?.appendChild(advanced);

    let default_btn = document.getElementById("load_default")!;
    default_btn.removeAttribute("disabled");
    default_btn.addEventListener('click', () => {
      CustTable.forEach(i => i.setElementDefaultValue());
    });

    this.toggleLoadLastSavedBtn(this.storage.load() !== null);

    let save_btn = document.getElementById("save")!;
    save_btn.removeAttribute("disabled");
    save_btn.addEventListener('click', () => {
      try {
        this.save();
      } catch (e) {
        console.error(e);
        alert(e);
      }
    });
  }

  initCustTabs() {
    const custTabs = Array.from(document.querySelectorAll(`nav[role="tablist"] > button`)) as HTMLElement[];
    custTabs.forEach(tab => {
      tab.removeAttribute("disabled");
      let platform = Number(tab.getAttribute("data-platform")!) as CustPlatform;
      tab.addEventListener('click', (_evt) => {
        // Set other tabs to unfocused state
        const unfocusedClasses = ["outline"];
        tab.classList.remove(...unfocusedClasses);
        let otherTabs = custTabs.filter(j => j != tab);
        otherTabs.forEach(k => k.classList.add(...unfocusedClasses));

        CustTable.forEach(e => {
          e.isAvailableFor(platform) ? e.showElement() : e.hideElement();
        });
      });
    });
  }

  parse() {
    let offset = this.beginOffset + this.magicLen;
    let revLen = 4;
    let rev = this.mapper[revLen].get(offset);
    if (rev != CUST_REV) {
      throw new Error(`Unsupported custRev, expected: ${CUST_REV}, got ${rev}`);
    }
    offset += revLen;
    document.getElementById("cust_rev")!.innerHTML = `Cust v${CUST_REV} is loaded.`;

    CustTable.forEach(i => {
      i.offset = offset;
      let mapper = this.mapper[i.size];
      if (!mapper) {
        i.getInputElement()?.focus();
        throw new Error(`Unknown size at ${i}`);
      }
      i.value = mapper.get(offset);
      offset += i.size;
      i.validate();
    });
  }

  load(buffer: ArrayBuffer) {
    try {
      this.buffer = buffer;
      this.findMagicOffset();
      this.removeHTMLForm();
      this.parse();
      this.initCustTabs();
      this.createHTMLForm();
    } catch (e) {
      console.error(e);
      alert(e);
    }
  }
}

/* GitHub Release fetch */
class ReleaseAsset {
  downloadUrl: string;
  updatedAt: string;

  constructor (obj: { browser_download_url: string; updated_at: string; }) {
    this.downloadUrl = obj.browser_download_url;
    this.updatedAt = obj.updated_at;
  };
};

class ReleaseInfo {
  ocVer: string;
  amsVer: string;
  loaderKipAsset: ReleaseAsset;
  sdOutZipAsset: ReleaseAsset;
  amsUrl: string;

  readonly ocLatestApi = "https://api.github.com/repos/hanai3Bi/Switch-OC-Suite/releases/latest";

  async load() {
    try {
      this.parseOcResponse(await this.responseFromApi(this.ocLatestApi).catch());
    } catch (e) {
      console.error(e);
      alert(e);
    }
  };

  async responseFromApi(apiUrl: string) : Promise<any> {
    const response = await fetch(apiUrl, { method: 'GET', headers: { Accept: 'application/json' } } );
    if (response.ok) {
      return await response.json();
    }

    throw new Error(`Failed to connect to "${apiUrl}": ${response.status}`);
  };

  parseOcResponse(response) {
    this.ocVer = response.tag_name;
    this.amsVer = this.ocVer.split(".").slice(0, 3).join(".");
    this.loaderKipAsset = new ReleaseAsset(response.assets.filter( a => a.name.endsWith("loader.kip") )[0]);
    this.sdOutZipAsset  = new ReleaseAsset(response.assets.filter( a => a.name.endsWith(".zip") )[0]);
    this.amsUrl = `https://github.com/Atmosphere-NX/Atmosphere/releases/tags/${this.amsVer}`;
  };
};

class DownloadSection {
  readonly element: HTMLElement = document.getElementById("download_btn_grid")!;

  async load() {
    while(!this.isVisible()) {
      await new Promise(r => setTimeout(r, 1000));
    }

    const info = new ReleaseInfo()
    await info.load();
    this.update("loader_kip_btn", `loader.kip <b>${info.ocVer}</b><br>${info.loaderKipAsset.updatedAt}`, info.loaderKipAsset.downloadUrl);
    this.update("sdout_zip_btn", `SdOut.zip <b>${info.ocVer}</b><br>${info.sdOutZipAsset.updatedAt}`, info.sdOutZipAsset.downloadUrl);
    this.update("ams_btn", `Atmosphere-NX <b>${info.amsVer}</b>`, info.amsUrl);
  }

  isVisible(): boolean {
    let rect = this.element.getBoundingClientRect();
    return (
      rect.top > 0 &&
      rect.left > 0 &&
      rect.bottom - rect.height < (window.innerHeight || document.documentElement.clientHeight) &&
      rect.right - rect.width < (window.innerWidth || document.documentElement.clientWidth)
    );
  }

  update(id: string, name: string, url: string) {
    let element = document.getElementById(id)!;
    element.innerHTML = name;
    element.removeAttribute("aria-busy");
    element.setAttribute("href", url);
  }
}

const fileInput = document.getElementById("file") as HTMLInputElement;
fileInput.addEventListener('change', (event) => {
  var cust: Cust = new Cust();
  // User canceled or non files selected
  if (!event.target || !(event.target as HTMLInputElement).files) {
    return;
  }
  let reader = new FileReader();
  reader.readAsArrayBuffer((event.target as HTMLInputElement).files![0]);
  reader.onloadend = (progEvent) => {
    if (progEvent.target!.readyState == FileReader.DONE) {
      cust.load(progEvent.target!.result! as ArrayBuffer);
    }
  };
});

addEventListener('DOMContentLoaded', async (_evt) => {
  await new DownloadSection().load();
});
