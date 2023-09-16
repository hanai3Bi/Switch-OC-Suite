/* Config: Cust */
const CUST_REV_ADV = 10;

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

class AdvEntry extends CustEntry {
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

      document.getElementById("config-list-advanced")!.appendChild(grid);

      new ErrorToolTip(this.id).addChangeListener();
    }
    input.value = String(this.value);
  }
}

class GpuEntry extends CustEntry {
  constructor(
    public id: string,
    public name: string,
    public platform: CustPlatform = CustPlatform.Mariko,
    public size: number = 4,
    public desc: string[] = ["range: 610 ≤ x ≤ 1000"],
    public defval: number = 610,
    minmax: [number, number] = [610, 1000],
    public step: number = 5,
    public zeroable: boolean = false) {
      super(id, name, platform, size, desc, defval, minmax, step, zeroable);
  };
  
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

      document.getElementById("config-list-gpu")!.appendChild(grid);

      new ErrorToolTip(this.id).addChangeListener();
    }
    input.value = String(this.value);
  }
}

var CustTable: Array<CustEntry> = [
  new CustEntry(
    "mtcConf",
    "DRAM Timing",
    CustPlatform.All,
    4,
    ["<b>0</b>: AUTO_ADJ_ALL: Auto adjust timings with LPDDR4 3733 Mbps specs, 8Gb density. Change timing with Advanced Config (Default)",
     "<b>1</b>: CUSTOM_ADJ_ALL: Adjust only non-zero preset timings in Advanced Config",
     "<b>2</b>: NO_ADJ_ALL: No timing adjustment (Timing becomes tighter if you raise dram clock)  Might achieve better performance but lower maximum frequency is expected."],
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
     "Official lpddr4(x) range: 1060mV~1175mV",
     "Public version needs high voltage because of wrong values, but it is recommended to stay within safe limits",
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
     "Recommended Clocks: 1862400, 2131200 (JEDEC)",
     "<b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM"],
    1862_400,
    [1600_000, 2131_200],
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
     "Recommended Clocks: 1862400, 2131200, 2400000 (JEDEC)",
     "Clocks above 2400Mhz might not boot, or work correctly",
     "<b>WARNING:</b> RAM overclock could be UNSTABLE if timing parameters are not suitable for your DRAM."],
    1996_800,
    [1600_000, 2502_400],
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
     "Micron Official lpddr4(x) range: 570mV~650mV",
     "Not enabled by default.",
     "This will not work without sys-clk-OC."],
    0,
    [550_000, 650_000],
    5000,
  ),
  new CustEntry(
    "marikoCpuUV",
    "Enable Mariko CPU Undervolt",
    CustPlatform.Mariko,
    4,
    ["Reduce CPU power draw",
     "Your CPU might not withstand undervolt and performance might drop",
     "<b>0</b> : Default Table",
     "<b>1</b> : Undervolt Level 1 (SLT - CPU speedo < 1650)",
     "<b>2</b> : Undervolt Level 1 (SLT - CPU speedo >= 1650)",],
     0,
     [0,2],
     1,
  ),
  new CustEntry(
    "marikoGpuUV",
    "Enable Mariko GPU Undervolt",
    CustPlatform.Mariko,
    4,
    ["Reduce GPU power draw",
     "Your GPU might not withstand undervolt and may not work properly",
     "Can hang your console, or crash games",
     "<b>0</b> : Default Table",
     "<b>1</b> : Undervolt Level 1 (SLT: Aggressive)",
     "<b>2</b> : Undervolt Level 2 (HiOPT: Drastic)",
     "<b>3</b> : Custom static GPU Table (Use Gpu Configuation below)"],
     0,
     [0,3],
     1,
  ),
];

var AdvTable: Array<AdvEntry> = [
  new AdvEntry(
    "marikoEmcDvbShift",
    "Step up Mariko EMC DVB Table",
    CustPlatform.Mariko,
    4,
    ["Might help with stability at higher memory clock",
     "<b>0</b> : Don't Adjust",
     "<b>1</b> : Shift one step",
     "<b>2</b> : Shift two step"],
     0,
     [0,2],
     1,
  ),
  new AdvEntry(
    "ramTimingPresetOne",
    "Primary RAM Timing Preset",
    CustPlatform.Mariko,
    4,
    ["<b>WARNING</b>: Unstable timings can corrupt your nand",
     "Select Timing Preset for both AUTO_ADJ and CUSTOM_ADJ",
     "Values are : tRCD - tRP - tRAS (tRC = tRP + tRAS)",
     "<b>0</b> : Do Not Adjust (2400Mhz: 12 - 12 - 28) (CUST_ADJ only)",
     "<b>1</b> : 18 - 18 - 42 (Default timing)",
     "<b>2</b> : 17 - 17 - 39",
     "<b>3</b> : 16 - 16 - 36",
     "<b>4</b> : 15 - 15 - 34",
     "<b>5</b> : 14 - 14 - 32",
     "<b>6</b> : 13 - 13 - 30"],
     1,
     [0,6],
     1,
  ),
  new AdvEntry(
    "ramTimingPresetTwo",
    "Secondary RAM Timing Preset",
    CustPlatform.Mariko,
    4,
    ["WARNING: Unstable timings can corrupt your nand",
     "Secondary Timing Preset for both AUTO_ADJ and CUSTOM_ADJ",
     "Values are : tRRD - tFAW",
     "<b>0</b> : Do Not Adjust (2400Mhz: 6.6 - 26.6) (CUST_ADJ only)",
     "<b>1</b> : 10 - 40 (Default timing) (3733 specs)", 
     "<b>2</b> : 7.5 - 30 (4266 specs)",
     "<b>3</b> : 6 - 24",
     "<b>4</b> : 4 - 16",
     "<b>5</b> : 3 - 12",],
     1,
     [0,5],
     1,
  ),
  new AdvEntry(
    "ramTimingPresetThree",
    "Secondary RAM Timing Preset",
    CustPlatform.Mariko,
    4,
    ["WARNING: Unstable timings can corrupt your nand",
     "Secondary Timing Preset for both AUTO_ADJ and CUSTOM_ADJ",
     "Values are : tWR - tRTP",
     "<b>0</b> : Do Not Adjust (2400Mhz: ?? - 5) (CUST_ADJ only)",
     "<b>1</b> : 18 - 7.5 (Default timing)", 
     "<b>2</b> : 15 - 7.5",
     "<b>3</b> : 15 - 6",
     "<b>4</b> : 12 - 6",
     "<b>5</b> : 12 - 4",
     "<b>6</b> : 8 - 4",],
     1,
     [0,6],
     1,
  ),
  new AdvEntry(
    "ramTimingPresetFour",
    "Secondary RAM Timing Preset",
    CustPlatform.Mariko,
    4,
    ["WARNING: Unstable timings can corrupt your nand",
     "Secondary Timing Preset for both AUTO_ADJ and CUSTOM_ADJ",
     "Values are : tRFC",
     "<b>0</b> : Do Not Adjust (2400Mhz: 93.3) (CUST_ADJ only)",
     "<b>1</b> : 140 (Default timing)", 
     "<b>2</b> : 120",
     "<b>3</b> : 100",
     "<b>4</b> : 80",
     "<b>5</b> : 70",
     "<b>6</b> : 60",],
     1,
     [0,6],
     1,
  ),
  new AdvEntry(
    "ramTimingPresetFive",
    "Secondary RAM Timing Preset",
     CustPlatform.Mariko,
     4,
     ["WARNING: Unstable timings can corrupt your nand",
     "Secondary Timing Preset for both AUTO_ADJ and CUSTOM_ADJ",
     "Values are : tWTR",
     "<b>0</b> : Do Not Adjust (2400Mhz: ??) (CUST_ADJ only)",
     "<b>1</b> : 10 (Default timing)", 
     "<b>2</b> : 8",
     "<b>3</b> : 6",
     "<b>4</b> : 4",
     "<b>5</b> : 2",
     "<b>6</b> : 1",],
     1,
     [0,6],
     1,
  ),
  new AdvEntry(
    "ramTimingPresetSix",
    "Tertiary RAM Timing Preset",
     CustPlatform.Mariko,
     4,
     ["WARNING: Unstable timings can corrupt your nand",
     "Tertiary Timing Preset for both AUTO_ADJ and CUSTOM_ADJ",
     "Values are : tREFpb",
     "<b>0</b> : Do Not Adjust (2400Mhz: 325) (CUST_ADJ only)",
     "<b>1</b> : 488 (Default timing)", 
     "<b>2</b> : 976",
     "<b>3</b> : 1952",
     "<b>4</b> : 3256",
     "<b>5</b> : MAX",],
     1,
     [0,5],
     1,
  ),
  new AdvEntry(
    "ramTimingPresetSeven",
    "Latency Decrement",
     CustPlatform.Mariko,
     4,
     ["WARNING: Unstable timings can corrupt your nand",
     "Latency decrement for both AUTO_ADJ and CUSTOM_ADJ",
     "This preset decreases Write/Read related delays. Values are Write - Read",
     "<b>0</b> : 0 - 0, Do Not Adjust for CUST_ADJ",
     "<b>1</b> : '-2' - '-4'",
     "<b>2</b> : '-4' - '-8'",
     "<b>3</b> : '-6' - '-12'",
     "<b>4</b> : '-8' - '-16'",
     "<b>5</b> : '-10' - '-20'",
     "<b>6</b> : '-12' - '-24'",],
     0,
     [0,6],
     1,
  )
];

var GpuTable: Array<GpuEntry> = [
  new GpuEntry("0", "76.8",),
  new GpuEntry("1", "153.6",),
  new GpuEntry("2", "230.4",),
  new GpuEntry("3", "307.2",),
  new GpuEntry("4", "384.0",),
  new GpuEntry("5", "460.8",),
  new GpuEntry("6", "537.6",),
  new GpuEntry("7", "614.4",),
  new GpuEntry("8", "691.2",),
  new GpuEntry("9", "768.0",),
  new GpuEntry("10", "844.8",),
  new GpuEntry("11", "921.6",),
  new GpuEntry("12", "998.4",),
  new GpuEntry("13", "1075.2",),
  new GpuEntry("14", "1152.0",),
  new GpuEntry("15", "1228.8",),
  new GpuEntry("16", "1267.2",),
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
    let update = (i => {
      i.updateValueFromElement();
      if (!i.validate()) {
        i.getInputElement()?.focus();
        throw new Error(`Invalid ${i.name}`);
      }
    });
    CustTable.forEach(update);
    AdvTable.forEach(update);
    GpuTable.forEach(update);
    
    this.storage = {};
    let kv = Object.fromEntries(CustTable.map((i) => [i.id, i.value]));
    Object.keys(kv)
          .forEach(k => this.storage[k] = kv[k]);
    kv = Object.fromEntries(AdvTable.map((i) => [i.id, i.value]));
    Object.keys(kv)
          .forEach(k => this.storage[k] = kv[k]);
  }

  setTable() {
    let keys = Object.keys(this.storage);
    keys.forEach(k => CustTable.filter(i => i.id == k)[0].value = this.storage[k]);
    keys.forEach(k => AdvTable.filter(i => i.id == k)[0].value = this.storage[k]);

    // Set default for missing values
    CustTable.filter(i => !keys.includes(i.id))
             .forEach(i => i.value = i.defval);
    AdvTable.filter(i => !keys.includes(i.id))
             .forEach(i => i.value = i.defval);

    CustTable.forEach(i => {
      if (!i.validate()) {
        i.getInputElement()?.focus();
        throw new Error(`Invalid ${i.name}`);
      }
      i.setElementValue();
    });
    AdvTable.forEach(i => {
      if (!i.validate()) {
        i.getInputElement()?.focus();
        throw new Error(`Invalid ${i.name}`);
      }
      i.setElementValue();
    });
    GpuTable.forEach(i => {
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
    keys = AdvTable.map(i => i.id);
    ignoredKeys = Object.keys(dict).filter(k => !keys.includes(k));
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
  rev: number;

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
    let saveValue = (i => {
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
    CustTable.forEach(saveValue);
    AdvTable.forEach(saveValue);
    GpuTable.forEach(saveValue);
    
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
    advanced.innerHTML = "Advanced configuration";
    document.getElementById("config-list-advanced")?.appendChild(advanced);

    let gpu = document.createElement("p");
    gpu.innerHTML = "Gpu Volt configuration";
    document.getElementById("config-list-gpu")?.appendChild(gpu);

    AdvTable.forEach(i => i.createElement());
    GpuTable.forEach(i => i.createElement());

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
    this.rev = this.mapper[revLen].get(offset);
    if (this.rev != CUST_REV_ADV) {
      throw new Error(`Unsupported custRev, expected: ${CUST_REV_ADV}, got ${this.rev}`);
    }
    offset += revLen;
    document.getElementById("cust_rev")!.innerHTML = `Cust v${this.rev} is loaded.`;

    let loadValue = (i => {
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

    CustTable.forEach(loadValue);
    AdvTable.forEach(loadValue);
    GpuTable.forEach(loadValue);
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
