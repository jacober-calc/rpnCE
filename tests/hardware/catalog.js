(function (root) {
'use strict';
// Menu snapshot and decimal references; functional cases follow below.
const source = {
  "groups": [
    {"id":"HOME","label":"HOME","path":["HOME"],"route":["MATH"],"rows":[["MATH","NUM","PROB","CPLX","TVM"],["BASE","HYP","STAT","CONV","HELP"],["CLR","MODE","STK","UNIT","DATE"]],"section":"HOME"},
    {"id":"MATH","label":"MATH","path":["HOME","MATH"],"route":["MATH","Y="],"rows":[["X^3","³√X","X√Y","ATN2","—"],["2^X","LOG2","LOGB","E^X-","LN1+"]],"section":"MATH"},
    {"id":"NUM","label":"NUM","path":["HOME","NUM"],"route":["MATH","WINDOW"],"rows":[["%","%CH","GCD","LCM","PRIM"],["IP","FP","FLR","CEIL","NEAR"],["ABS","SIGN","MOD","MIN","MAX"],["RNDF","RNDN","RNDG","—","—"]],"section":"NUM"},
    {"id":"PROB","label":"PROB","path":["HOME","PROB"],"route":["MATH","ZOOM"],"rows":[["COMB","PERM","N!","RAND","SEED"]],"section":"PROB"},
    {"id":"COMPLEX","label":"CPLX","path":["HOME","CPLX"],"route":["MATH","TRACE"],"rows":[["REAL","IMAG","CONJ","CABS","CANG"]],"section":"CPLX"},
    {"id":"TVM","label":"TVM","path":["HOME","TVM"],"route":["MATH","GRAPH"],"rows":[["N","I%YR","PV","PMT","FV"],["P/YR","C/YR","BEG","END","CLTV"],["IYR1","IYR2","TMAX","—","RSTV"]],"section":"TVM"},
    {"id":"BASE","label":"BASE","path":["HOME","BASE"],"route":["MATH","↓","Y="],"rows":[["DEC","HEX","OCT","BIN","—"],["LOGI","ROTS","BITS","BFCN","BCFS"]],"section":"BASE"},
    {"id":"BASE_LOGIC","label":"LOGI","path":["HOME","BASE","LOGI"],"route":["MATH","↓","Y=","↓","Y="],"rows":[["AND","OR","XOR","NOT","NEG"]],"section":"BASE"},
    {"id":"BASE_ROTATE","label":"ROTS","path":["HOME","BASE","ROTS"],"route":["MATH","↓","Y=","↓","WINDOW"],"rows":[["SL","SR","ASR","SLn","SRn"],["RL","RR","RLC","RRC","—"],["RLn","RRn","RLCn","RRCn","—"]],"section":"BASE"},
    {"id":"BASE_BITS","label":"BITS","path":["HOME","BASE","BITS"],"route":["MATH","↓","Y=","↓","ZOOM"],"rows":[["CB","SB","B?","REVB","CNTB"]],"section":"BASE"},
    {"id":"BASE_FUNCTIONS","label":"BFCN","path":["HOME","BASE","BFCN"],"route":["MATH","↓","Y=","↓","TRACE"],"rows":[["BAS+","BAS-","BAS×","BAS/","BDIV"]],"section":"BASE"},
    {"id":"BASE_CONFIGS","label":"BCFS","path":["HOME","BASE","BCFS"],"route":["MATH","↓","Y=","↓","GRAPH"],"rows":[["CCF","SCF","CF?","WSIZ","WSZ?"]],"section":"BASE"},
    {"id":"HYPERBOLIC","label":"HYP","path":["HOME","HYP"],"route":["MATH","↓","WINDOW"],"rows":[["—","SINH","COSH","TANH","—"],["—","ASNH","ACSH","ATNH","—"]],"section":"HYP"},
    {"id":"STAT","label":"STAT","path":["HOME","STAT"],"route":["MATH","↓","ZOOM"],"rows":[["Σ+","Σ-","ALLΣ","LINΣ","CLΣ"],["SUM","MEAN","WMN","N","Σ"],["SDEV","SCOV","PDEV","PCOV","CFIT"]],"section":"STAT"},
    {"id":"STAT_REGS","label":"Σ","path":["HOME","STAT","Σ"],"route":["MATH","↓","ZOOM","↓","GRAPH"],"rows":[["ΣX","ΣX2","ΣY","ΣY2","ΣXY"],["ΣN","ΣLX","ΣLX2","ΣLY","ΣLY2"],["ΣLXL","ΣXLY","ΣYLX","—","—"]],"section":"STAT"},
    {"id":"CFIT","label":"CFIT","path":["HOME","STAT","CFIT"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH"],"rows":[["Y→X","X→Y","SLOP","YINT","CORR"],["LINF","LOGF","EXPF","PWRF","BEST"]],"section":"STAT"},
    {"id":"CONV","label":"CONV","path":["HOME","CONV"],"route":["MATH","↓","TRACE"],"rows":[["→DEG","→RAD","—","→REC","→POL"],["→HR","→HMS","HMS+","HMS-","—"]],"section":"CONV"},
    {"id":"CLEAR","label":"CLR","path":["HOME","CLR"],"route":["MATH","↓","↓","Y="],"rows":[["CLX","CLST","CLRG","CLΣ","CLTV"],["CLD","—","—","—","—"]],"section":"CLR"},
    {"id":"MODE","label":"MODE","path":["HOME","MODE"],"route":["MATH","↓","↓","WINDOW"],"rows":[["FIX","SCI","ENG","RAD","DEG"],["RRES","CRES","RECT","PRAD","PDEG"],["RSIZ","RSZ?","TINT","SSIZ","SSZ?"],["{..}","\"..\"",".",",","—"]],"section":"MODE"},
    {"id":"STACK","label":"STK","path":["HOME","STK"],"route":["MATH","↓","↓","ZOOM"],"rows":[["DUP","R↑","R↓","DROP","X←→Y"],["X<>","—","—","—","—"]],"section":"STK"},
    {"id":"UNIT","label":"UNIT","path":["HOME","UNIT"],"route":["MATH","↓","↓","TRACE"],"rows":[["UFCN","LENG","AREA","VOL","TEMP"],["MASS","FORC","PRES","ENER","PWR"],["TIME","SPD","FUEL","—","—"]],"section":"UNIT"},
    {"id":"UNIT_FUNCTIONS","label":"UFCN","path":["HOME","UNIT","UFCN"],"route":["MATH","↓","↓","TRACE","Y="],"rows":[["UVAL","UBAS","—","—","—"]],"section":"UNIT"},
    {"id":"UNIT_LENGTH","label":"LENG","path":["HOME","UNIT","LENG"],"route":["MATH","↓","↓","TRACE","WINDOW"],"rows":[["ferm","angs","nm","µm","—"],["mm","cm","m","km","—"],["mil","in","ft","yd","mi"],["TYPO","SURV","NAUT","ASTR","—"]],"section":"UNIT"},
    {"id":"TYPOGRAPHY","label":"TYPO","path":["HOME","UNIT","LENG","TYPO"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","Y="],"rows":[["twip","pt","pica","—","—"]],"section":"UNIT"},
    {"id":"SURVEY","label":"SURV","path":["HOME","UNIT","LENG","SURV"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","WINDOW"],"rows":[["svft","rod","chai","frlg","svmi"],["leag","—","—","—","—"]],"section":"UNIT"},
    {"id":"NAUTICAL","label":"NAUT","path":["HOME","UNIT","LENG","NAUT"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","ZOOM"],"rows":[["fath","cabl","nmi","—","—"]],"section":"UNIT"},
    {"id":"ASTRONOMY","label":"ASTR","path":["HOME","UNIT","LENG","ASTR"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","TRACE"],"rows":[["lsec","AU","ly","pc","—"]],"section":"UNIT"},
    {"id":"UNIT_AREA","label":"AREA","path":["HOME","UNIT","AREA"],"route":["MATH","↓","↓","TRACE","ZOOM"],"rows":[["mm²","cm²","m²","km²","—"],["in²","ft²","yd²","mi²","nmi²"],["acre","ha","usfb","cafb","SURV"]],"section":"UNIT"},
    {"id":"UNIT_AREA_SURV","label":"SURV","path":["HOME","UNIT","AREA","SURV"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","↓","GRAPH"],"rows":[["rod²","chn²","fur²","—","—"]],"section":"UNIT"},
    {"id":"UNIT_VOL","label":"VOL","path":["HOME","UNIT","VOL"],"route":["MATH","↓","↓","TRACE","TRACE"],"rows":[["mm³","cm³","m³","km³","—"],["in³","ft³","yd³","mi³","nmi³"],["µL","mL","L","mtsp","mtbs"],["US","IMP","DRY","MISC","—"]],"section":"UNIT"},
    {"id":"UNIT_VOL_US","label":"US","path":["HOME","UNIT","VOL","US"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","Y="],"rows":[["tsp","tbsp","floz","gill","cup"],["pint","qt","gal","—","—"]],"section":"UNIT"},
    {"id":"UNIT_VOL_IMP","label":"IMP","path":["HOME","UNIT","VOL","IMP"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","WINDOW"],"rows":[["tsp","tbsp","floz","gill","cup"],["pint","qt","gal","—","—"]],"section":"UNIT"},
    {"id":"UNIT_VOL_DRY","label":"DRY","path":["HOME","UNIT","VOL","DRY"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","ZOOM"],"rows":[["drpt","drqt","dgal","peck","bush"],["dbbl","—","—","—","—"]],"section":"UNIT"},
    {"id":"UNIT_VOL_MISC","label":"MISC","path":["HOME","UNIT","VOL","MISC"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","TRACE"],"rows":[["bdft","bbl","olmp","acft","—"]],"section":"UNIT"},
    {"id":"UNIT_TEMP","label":"TEMP","path":["HOME","UNIT","TEMP"],"route":["MATH","↓","↓","TRACE","GRAPH"],"rows":[["°C","°F","°R","°K","—"]],"section":"UNIT"},
    {"id":"UNIT_MASS","label":"MASS","path":["HOME","UNIT","MASS"],"route":["MATH","↓","↓","TRACE","↓","Y="],"rows":[["µg","mg","g","kg","t"],["amu","US","IMP","TROY","—"]],"section":"UNIT"},
    {"id":"UNIT_MASS_US","label":"US","path":["HOME","UNIT","MASS","US"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","WINDOW"],"rows":[["grai","dram","oz","lb","slug"],["scwt","ston","—","—","—"]],"section":"UNIT"},
    {"id":"UNIT_MASS_IMP","label":"IMP","path":["HOME","UNIT","MASS","IMP"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","ZOOM"],"rows":[["lb","stne","qrtr","lcwt","lton"]],"section":"UNIT"},
    {"id":"UNIT_MASS_TROY","label":"TROY","path":["HOME","UNIT","MASS","TROY"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","TRACE"],"rows":[["grai","dwt","ozt","lbt","—"]],"section":"UNIT"},
    {"id":"UNIT_FORCE","label":"FORC","path":["HOME","UNIT","FORC"],"route":["MATH","↓","↓","TRACE","↓","WINDOW"],"rows":[["dyne","N","kgf","tonf","—"],["pdl","lbf","stnf","ltnf","—"]],"section":"UNIT"},
    {"id":"UNIT_PRESSURE","label":"PRES","path":["HOME","UNIT","PRES"],"route":["MATH","↓","↓","TRACE","↓","ZOOM"],"rows":[["Pa","hPa","kPa","torr","atm"],["mbar","dbar","bar","psi","—"],["mmH","inHg","mmw","inwg","—"]],"section":"UNIT"},
    {"id":"UNIT_ENERGY","label":"ENER","path":["HOME","UNIT","ENER"],"route":["MATH","↓","↓","TRACE","↓","TRACE"],"rows":[["eV","erg","J","kJ","ftlb"],["Wh","kWh","cal","kcal","Btu"],["tTNT","Latm","—","—","—"]],"section":"UNIT"},
    {"id":"UNIT_POWER","label":"PWR","path":["HOME","UNIT","PWR"],"route":["MATH","↓","↓","TRACE","↓","GRAPH"],"rows":[["W","kW","fl/s","Bt/h","Bt/m"],["ca/s","hp","—","—","—"]],"section":"UNIT"},
    {"id":"UNIT_TIME","label":"TIME","path":["HOME","UNIT","TIME"],"route":["MATH","↓","↓","TRACE","↓","↓","Y="],"rows":[["ns","µs","ms","sec","min"],["hour","day","week","mon","year"]],"section":"UNIT"},
    {"id":"UNIT_SPEED","label":"SPD","path":["HOME","UNIT","SPD"],"route":["MATH","↓","↓","TRACE","↓","↓","WINDOW"],"rows":[["m/s","ft/s","kph","mph","knot"],["c","—","—","—","—"]],"section":"UNIT"},
    {"id":"UNIT_FUEL","label":"FUEL","path":["HOME","UNIT","FUEL"],"route":["MATH","↓","↓","TRACE","↓","↓","ZOOM"],"rows":[["Lkm","mpg","—","—","—"]],"section":"UNIT"},
    {"id":"DATE","label":"DATE","path":["HOME","DATE"],"route":["MATH","↓","↓","GRAPH"],"rows":[["D","T","DT","TZ","DZ"],["DR","DW","EPCH","CLK","—"]],"section":"DATE"},
    {"id":"DATE_FOLDER_DATE","label":"D","path":["HOME","DATE","D"],"route":["MATH","↓","↓","GRAPH","Y="],"rows":[["D{}","→ED","ED→","→ES","ES→"],["LEAP","DOW","CVTZ","—","—"],[".YR",".MON",".DAY","—","—"],["DSHK","DEXD","DCUT","DLNK","—"]],"section":"DATE"},
    {"id":"DATE_FOLDER_TIME","label":"T","path":["HOME","DATE","T"],"route":["MATH","↓","↓","GRAPH","WINDOW"],"rows":[["T{}","→S","S→","—","—"],[".HR",".MIN",".SEC","—","—"]],"section":"DATE"},
    {"id":"DATE_FOLDER_DATE_TIME","label":"DT","path":["HOME","DATE","DT"],"route":["MATH","↓","↓","GRAPH","ZOOM"],"rows":[["DT{}","→ED","ED→","→ES","ES→"],["LEAP","DOW","CVTZ",".D{}",".T{}"],["DSHK","DEXD","DCUT","DLNK","—"]],"section":"DATE"},
    {"id":"DATE_FOLDER_OFFSET","label":"TZ","path":["HOME","DATE","TZ"],"route":["MATH","↓","↓","GRAPH","TRACE"],"rows":[["TZ{}","→HR","HR→",".HR",".MIN"]],"section":"DATE"},
    {"id":"DATE_FOLDER_OFFSET_DATE_TIME","label":"DZ","path":["HOME","DATE","DZ"],"route":["MATH","↓","↓","GRAPH","GRAPH"],"rows":[["DZ{}","→ED","ED→","→ES","ES→"],["ES→@","LEAP","DOW","CVTZ","—"],[".D{}",".T{}",".DT{",".TZ{","—"],["DSHK","DEXD","DCUT","DLNK","—"]],"section":"DATE"},
    {"id":"DATE_FOLDER_DURATION","label":"DR","path":["HOME","DATE","DR"],"route":["MATH","↓","↓","GRAPH","↓","Y="],"rows":[["DR{}","DAY→","HR→","MIN→","SEC→"],["→S",".DAY",".HR",".MIN",".SEC"]],"section":"DATE"},
    {"id":"DATE_FOLDER_DAY_OF_WEEK","label":"DW","path":["HOME","DATE","DW"],"route":["MATH","↓","↓","GRAPH","↓","WINDOW"],"rows":[["DW{}","→ISO","ISO→","→UNX","UNX→"]],"section":"DATE"},
    {"id":"EPOCH","label":"EPCH","path":["HOME","DATE","EPCH"],"route":["MATH","↓","↓","GRAPH","↓","ZOOM"],"rows":[["UNIX","NTP","GPS","TIOS","Y2K"],["CEPC","—","—","EPC","EPC?"]],"section":"DATE"},
    {"id":"CLK","label":"CLK","path":["HOME","DATE","CLK"],"route":["MATH","↓","↓","GRAPH","↓","TRACE"],"rows":[["NOW","NOWD","NOWT","NWDZ","NWUT"],["ATZ","ATZ?","CTZ","CTZ?","SETC"]],"section":"DATE"}
  ],
  "commands": [
    {"id":"CUBE","label":"X^3","path":["HOME","MATH","X^3"],"route":["MATH","Y=","Y="],"section":"MATH"},
    {"id":"CUBE_ROOT","label":"³√X","path":["HOME","MATH","³√X"],"route":["MATH","Y=","WINDOW"],"section":"MATH"},
    {"id":"XROOT_Y","label":"X√Y","path":["HOME","MATH","X√Y"],"route":["MATH","Y=","ZOOM"],"section":"MATH"},
    {"id":"ATAN2","label":"ATN2","path":["HOME","MATH","ATN2"],"route":["MATH","Y=","TRACE"],"section":"MATH"},
    {"id":"TWO_POW","label":"2^X","path":["HOME","MATH","2^X"],"route":["MATH","Y=","↓","Y="],"section":"MATH"},
    {"id":"LOG2","label":"LOG2","path":["HOME","MATH","LOG2"],"route":["MATH","Y=","↓","WINDOW"],"section":"MATH"},
    {"id":"LOG_BASE","label":"LOGB","path":["HOME","MATH","LOGB"],"route":["MATH","Y=","↓","ZOOM"],"section":"MATH"},
    {"id":"EXP_MINUS_ONE","label":"E^X-","path":["HOME","MATH","E^X-"],"route":["MATH","Y=","↓","TRACE"],"section":"MATH"},
    {"id":"LN_ONE_PLUS","label":"LN1+","path":["HOME","MATH","LN1+"],"route":["MATH","Y=","↓","GRAPH"],"section":"MATH"},
    {"id":"PERCENT","label":"%","path":["HOME","NUM","%"],"route":["MATH","WINDOW","Y="],"section":"NUM"},
    {"id":"PERCENT_CHANGE","label":"%CH","path":["HOME","NUM","%CH"],"route":["MATH","WINDOW","WINDOW"],"section":"NUM"},
    {"id":"GCD","label":"GCD","path":["HOME","NUM","GCD"],"route":["MATH","WINDOW","ZOOM"],"section":"NUM"},
    {"id":"LCM","label":"LCM","path":["HOME","NUM","LCM"],"route":["MATH","WINDOW","TRACE"],"section":"NUM"},
    {"id":"PRIME","label":"PRIM","path":["HOME","NUM","PRIM"],"route":["MATH","WINDOW","GRAPH"],"section":"NUM"},
    {"id":"INT_PART","label":"IP","path":["HOME","NUM","IP"],"route":["MATH","WINDOW","↓","Y="],"section":"NUM"},
    {"id":"FRAC_PART","label":"FP","path":["HOME","NUM","FP"],"route":["MATH","WINDOW","↓","WINDOW"],"section":"NUM"},
    {"id":"FLOOR","label":"FLR","path":["HOME","NUM","FLR"],"route":["MATH","WINDOW","↓","ZOOM"],"section":"NUM"},
    {"id":"CEIL","label":"CEIL","path":["HOME","NUM","CEIL"],"route":["MATH","WINDOW","↓","TRACE"],"section":"NUM"},
    {"id":"NEAR","label":"NEAR","path":["HOME","NUM","NEAR"],"route":["MATH","WINDOW","↓","GRAPH"],"section":"NUM"},
    {"id":"ABS","label":"ABS","path":["HOME","NUM","ABS"],"route":["MATH","WINDOW","↓","↓","Y="],"section":"NUM"},
    {"id":"SIGN","label":"SIGN","path":["HOME","NUM","SIGN"],"route":["MATH","WINDOW","↓","↓","WINDOW"],"section":"NUM"},
    {"id":"MOD","label":"MOD","path":["HOME","NUM","MOD"],"route":["MATH","WINDOW","↓","↓","ZOOM"],"section":"NUM"},
    {"id":"MIN","label":"MIN","path":["HOME","NUM","MIN"],"route":["MATH","WINDOW","↓","↓","TRACE"],"section":"NUM"},
    {"id":"MAX","label":"MAX","path":["HOME","NUM","MAX"],"route":["MATH","WINDOW","↓","↓","GRAPH"],"section":"NUM"},
    {"id":"ROUND_TO_FIX","label":"RNDF","path":["HOME","NUM","RNDF"],"route":["MATH","WINDOW","↓","↓","↓","Y="],"section":"NUM"},
    {"id":"ROUND_TO_N","label":"RNDN","path":["HOME","NUM","RNDN"],"route":["MATH","WINDOW","↓","↓","↓","WINDOW"],"section":"NUM"},
    {"id":"ROUND_TO_GUARD","label":"RNDG","path":["HOME","NUM","RNDG"],"route":["MATH","WINDOW","↓","↓","↓","ZOOM"],"section":"NUM"},
    {"id":"COMB","label":"COMB","path":["HOME","PROB","COMB"],"route":["MATH","ZOOM","Y="],"section":"PROB"},
    {"id":"PERM","label":"PERM","path":["HOME","PROB","PERM"],"route":["MATH","ZOOM","WINDOW"],"section":"PROB"},
    {"id":"FACTORIAL","label":"N!","path":["HOME","PROB","N!"],"route":["MATH","ZOOM","ZOOM"],"section":"PROB"},
    {"id":"RANDOM","label":"RAND","path":["HOME","PROB","RAND"],"route":["MATH","ZOOM","TRACE"],"section":"PROB"},
    {"id":"RANDOM_SEED","label":"SEED","path":["HOME","PROB","SEED"],"route":["MATH","ZOOM","GRAPH"],"section":"PROB"},
    {"id":"COMPLEX_REAL","label":"REAL","path":["HOME","CPLX","REAL"],"route":["MATH","TRACE","Y="],"section":"CPLX"},
    {"id":"COMPLEX_IMAG","label":"IMAG","path":["HOME","CPLX","IMAG"],"route":["MATH","TRACE","WINDOW"],"section":"CPLX"},
    {"id":"COMPLEX_CONJ","label":"CONJ","path":["HOME","CPLX","CONJ"],"route":["MATH","TRACE","ZOOM"],"section":"CPLX"},
    {"id":"COMPLEX_ABS","label":"CABS","path":["HOME","CPLX","CABS"],"route":["MATH","TRACE","TRACE"],"section":"CPLX"},
    {"id":"COMPLEX_ANGLE","label":"CANG","path":["HOME","CPLX","CANG"],"route":["MATH","TRACE","GRAPH"],"section":"CPLX"},
    {"id":"TVM_N","label":"N","path":["HOME","TVM","N"],"route":["MATH","GRAPH","Y="],"section":"TVM"},
    {"id":"TVM_IYR","label":"I%YR","path":["HOME","TVM","I%YR"],"route":["MATH","GRAPH","WINDOW"],"section":"TVM"},
    {"id":"TVM_PV","label":"PV","path":["HOME","TVM","PV"],"route":["MATH","GRAPH","ZOOM"],"section":"TVM"},
    {"id":"TVM_PMT","label":"PMT","path":["HOME","TVM","PMT"],"route":["MATH","GRAPH","TRACE"],"section":"TVM"},
    {"id":"TVM_FV","label":"FV","path":["HOME","TVM","FV"],"route":["MATH","GRAPH","GRAPH"],"section":"TVM"},
    {"id":"TVM_PYR","label":"P/YR","path":["HOME","TVM","P/YR"],"route":["MATH","GRAPH","↓","Y="],"section":"TVM"},
    {"id":"TVM_CYR","label":"C/YR","path":["HOME","TVM","C/YR"],"route":["MATH","GRAPH","↓","WINDOW"],"section":"TVM"},
    {"id":"TVM_BEGIN","label":"BEG","path":["HOME","TVM","BEG"],"route":["MATH","GRAPH","↓","ZOOM"],"section":"TVM"},
    {"id":"TVM_END","label":"END","path":["HOME","TVM","END"],"route":["MATH","GRAPH","↓","TRACE"],"section":"TVM"},
    {"id":"TVM_CLEAR","label":"CLTV","path":["HOME","TVM","CLTV"],"route":["MATH","GRAPH","↓","GRAPH"],"section":"TVM"},
    {"id":"TVM_IYR0","label":"IYR1","path":["HOME","TVM","IYR1"],"route":["MATH","GRAPH","↓","↓","Y="],"section":"TVM"},
    {"id":"TVM_IYR1","label":"IYR2","path":["HOME","TVM","IYR2"],"route":["MATH","GRAPH","↓","↓","WINDOW"],"section":"TVM"},
    {"id":"TVM_ITER_MAX","label":"TMAX","path":["HOME","TVM","TMAX"],"route":["MATH","GRAPH","↓","↓","ZOOM"],"section":"TVM"},
    {"id":"TVM_SOLVER_RESET","label":"RSTV","path":["HOME","TVM","RSTV"],"route":["MATH","GRAPH","↓","↓","GRAPH"],"section":"TVM"},
    {"id":"DEC","label":"DEC","path":["HOME","BASE","DEC"],"route":["MATH","↓","Y=","Y="],"section":"BASE"},
    {"id":"HEX","label":"HEX","path":["HOME","BASE","HEX"],"route":["MATH","↓","Y=","WINDOW"],"section":"BASE"},
    {"id":"OCT","label":"OCT","path":["HOME","BASE","OCT"],"route":["MATH","↓","Y=","ZOOM"],"section":"BASE"},
    {"id":"BIN","label":"BIN","path":["HOME","BASE","BIN"],"route":["MATH","↓","Y=","TRACE"],"section":"BASE"},
    {"id":"BITWISE_AND","label":"AND","path":["HOME","BASE","LOGI","AND"],"route":["MATH","↓","Y=","↓","Y=","Y="],"section":"BASE"},
    {"id":"BITWISE_OR","label":"OR","path":["HOME","BASE","LOGI","OR"],"route":["MATH","↓","Y=","↓","Y=","WINDOW"],"section":"BASE"},
    {"id":"BITWISE_XOR","label":"XOR","path":["HOME","BASE","LOGI","XOR"],"route":["MATH","↓","Y=","↓","Y=","ZOOM"],"section":"BASE"},
    {"id":"BITWISE_NOT","label":"NOT","path":["HOME","BASE","LOGI","NOT"],"route":["MATH","↓","Y=","↓","Y=","TRACE"],"section":"BASE"},
    {"id":"BITWISE_NEG","label":"NEG","path":["HOME","BASE","LOGI","NEG"],"route":["MATH","↓","Y=","↓","Y=","GRAPH"],"section":"BASE"},
    {"id":"SHIFT_LEFT_LOGICAL","label":"SL","path":["HOME","BASE","ROTS","SL"],"route":["MATH","↓","Y=","↓","WINDOW","Y="],"section":"BASE"},
    {"id":"SHIFT_RIGHT_LOGICAL","label":"SR","path":["HOME","BASE","ROTS","SR"],"route":["MATH","↓","Y=","↓","WINDOW","WINDOW"],"section":"BASE"},
    {"id":"SHIFT_RIGHT_ARITHMETIC","label":"ASR","path":["HOME","BASE","ROTS","ASR"],"route":["MATH","↓","Y=","↓","WINDOW","ZOOM"],"section":"BASE"},
    {"id":"SHIFT_LEFT_LOGICAL_N","label":"SLn","path":["HOME","BASE","ROTS","SLn"],"route":["MATH","↓","Y=","↓","WINDOW","TRACE"],"section":"BASE"},
    {"id":"SHIFT_RIGHT_LOGICAL_N","label":"SRn","path":["HOME","BASE","ROTS","SRn"],"route":["MATH","↓","Y=","↓","WINDOW","GRAPH"],"section":"BASE"},
    {"id":"ROTATE_LEFT_CIRCULAR","label":"RL","path":["HOME","BASE","ROTS","RL"],"route":["MATH","↓","Y=","↓","WINDOW","↓","Y="],"section":"BASE"},
    {"id":"ROTATE_RIGHT_CIRCULAR","label":"RR","path":["HOME","BASE","ROTS","RR"],"route":["MATH","↓","Y=","↓","WINDOW","↓","WINDOW"],"section":"BASE"},
    {"id":"ROTATE_LEFT_CARRY","label":"RLC","path":["HOME","BASE","ROTS","RLC"],"route":["MATH","↓","Y=","↓","WINDOW","↓","ZOOM"],"section":"BASE"},
    {"id":"ROTATE_RIGHT_CARRY","label":"RRC","path":["HOME","BASE","ROTS","RRC"],"route":["MATH","↓","Y=","↓","WINDOW","↓","TRACE"],"section":"BASE"},
    {"id":"ROTATE_LEFT_CIRCULAR_N","label":"RLn","path":["HOME","BASE","ROTS","RLn"],"route":["MATH","↓","Y=","↓","WINDOW","↓","↓","Y="],"section":"BASE"},
    {"id":"ROTATE_RIGHT_CIRCULAR_N","label":"RRn","path":["HOME","BASE","ROTS","RRn"],"route":["MATH","↓","Y=","↓","WINDOW","↓","↓","WINDOW"],"section":"BASE"},
    {"id":"ROTATE_LEFT_CARRY_N","label":"RLCn","path":["HOME","BASE","ROTS","RLCn"],"route":["MATH","↓","Y=","↓","WINDOW","↓","↓","ZOOM"],"section":"BASE"},
    {"id":"ROTATE_RIGHT_CARRY_N","label":"RRCn","path":["HOME","BASE","ROTS","RRCn"],"route":["MATH","↓","Y=","↓","WINDOW","↓","↓","TRACE"],"section":"BASE"},
    {"id":"CLEAR_BIT","label":"CB","path":["HOME","BASE","BITS","CB"],"route":["MATH","↓","Y=","↓","ZOOM","Y="],"section":"BASE"},
    {"id":"SET_BIT","label":"SB","path":["HOME","BASE","BITS","SB"],"route":["MATH","↓","Y=","↓","ZOOM","WINDOW"],"section":"BASE"},
    {"id":"GET_BIT","label":"B?","path":["HOME","BASE","BITS","B?"],"route":["MATH","↓","Y=","↓","ZOOM","ZOOM"],"section":"BASE"},
    {"id":"REVERSE_BIT","label":"REVB","path":["HOME","BASE","BITS","REVB"],"route":["MATH","↓","Y=","↓","ZOOM","TRACE"],"section":"BASE"},
    {"id":"COUNT_BIT","label":"CNTB","path":["HOME","BASE","BITS","CNTB"],"route":["MATH","↓","Y=","↓","ZOOM","GRAPH"],"section":"BASE"},
    {"id":"BASE_ADD","label":"BAS+","path":["HOME","BASE","BFCN","BAS+"],"route":["MATH","↓","Y=","↓","TRACE","Y="],"section":"BASE"},
    {"id":"BASE_SUBT","label":"BAS-","path":["HOME","BASE","BFCN","BAS-"],"route":["MATH","↓","Y=","↓","TRACE","WINDOW"],"section":"BASE"},
    {"id":"BASE_MULT","label":"BAS×","path":["HOME","BASE","BFCN","BAS×"],"route":["MATH","↓","Y=","↓","TRACE","ZOOM"],"section":"BASE"},
    {"id":"BASE_DIV","label":"BAS/","path":["HOME","BASE","BFCN","BAS/"],"route":["MATH","↓","Y=","↓","TRACE","TRACE"],"section":"BASE"},
    {"id":"BASE_DIV2","label":"BDIV","path":["HOME","BASE","BFCN","BDIV"],"route":["MATH","↓","Y=","↓","TRACE","GRAPH"],"section":"BASE"},
    {"id":"CLEAR_CARRY_FLAG","label":"CCF","path":["HOME","BASE","BCFS","CCF"],"route":["MATH","↓","Y=","↓","GRAPH","Y="],"section":"BASE"},
    {"id":"SET_CARRY_FLAG","label":"SCF","path":["HOME","BASE","BCFS","SCF"],"route":["MATH","↓","Y=","↓","GRAPH","WINDOW"],"section":"BASE"},
    {"id":"GET_CARRY_FLAG","label":"CF?","path":["HOME","BASE","BCFS","CF?"],"route":["MATH","↓","Y=","↓","GRAPH","ZOOM"],"section":"BASE"},
    {"id":"SET_WORD_SIZE","label":"WSIZ","path":["HOME","BASE","BCFS","WSIZ"],"route":["MATH","↓","Y=","↓","GRAPH","TRACE"],"section":"BASE"},
    {"id":"GET_WORD_SIZE","label":"WSZ?","path":["HOME","BASE","BCFS","WSZ?"],"route":["MATH","↓","Y=","↓","GRAPH","GRAPH"],"section":"BASE"},
    {"id":"SINH","label":"SINH","path":["HOME","HYP","SINH"],"route":["MATH","↓","WINDOW","WINDOW"],"section":"HYP"},
    {"id":"COSH","label":"COSH","path":["HOME","HYP","COSH"],"route":["MATH","↓","WINDOW","ZOOM"],"section":"HYP"},
    {"id":"TANH","label":"TANH","path":["HOME","HYP","TANH"],"route":["MATH","↓","WINDOW","TRACE"],"section":"HYP"},
    {"id":"ASINH","label":"ASNH","path":["HOME","HYP","ASNH"],"route":["MATH","↓","WINDOW","↓","WINDOW"],"section":"HYP"},
    {"id":"ACOSH","label":"ACSH","path":["HOME","HYP","ACSH"],"route":["MATH","↓","WINDOW","↓","ZOOM"],"section":"HYP"},
    {"id":"ATANH","label":"ATNH","path":["HOME","HYP","ATNH"],"route":["MATH","↓","WINDOW","↓","TRACE"],"section":"HYP"},
    {"id":"STAT_PLUS","label":"Σ+","path":["HOME","STAT","Σ+"],"route":["MATH","↓","ZOOM","Y="],"section":"STAT"},
    {"id":"STAT_MINUS","label":"Σ-","path":["HOME","STAT","Σ-"],"route":["MATH","↓","ZOOM","WINDOW"],"section":"STAT"},
    {"id":"STAT_ALL_MODE","label":"ALLΣ","path":["HOME","STAT","ALLΣ"],"route":["MATH","↓","ZOOM","ZOOM"],"section":"STAT"},
    {"id":"STAT_LINEAR_MODE","label":"LINΣ","path":["HOME","STAT","LINΣ"],"route":["MATH","↓","ZOOM","TRACE"],"section":"STAT"},
    {"id":"STAT_CLEAR","label":"CLΣ","path":["HOME","STAT","CLΣ"],"route":["MATH","↓","ZOOM","GRAPH"],"section":"STAT"},
    {"id":"STAT_SUM","label":"SUM","path":["HOME","STAT","SUM"],"route":["MATH","↓","ZOOM","↓","Y="],"section":"STAT"},
    {"id":"STAT_MEAN","label":"MEAN","path":["HOME","STAT","MEAN"],"route":["MATH","↓","ZOOM","↓","WINDOW"],"section":"STAT"},
    {"id":"STAT_WEIGHTED_MEAN","label":"WMN","path":["HOME","STAT","WMN"],"route":["MATH","↓","ZOOM","↓","ZOOM"],"section":"STAT"},
    {"id":"STAT_N","label":"N","path":["HOME","STAT","N"],"route":["MATH","↓","ZOOM","↓","TRACE"],"section":"STAT"},
    {"id":"STAT_REG_X","label":"ΣX","path":["HOME","STAT","Σ","ΣX"],"route":["MATH","↓","ZOOM","↓","GRAPH","Y="],"section":"STAT"},
    {"id":"STAT_REG_X2","label":"ΣX2","path":["HOME","STAT","Σ","ΣX2"],"route":["MATH","↓","ZOOM","↓","GRAPH","WINDOW"],"section":"STAT"},
    {"id":"STAT_REG_Y","label":"ΣY","path":["HOME","STAT","Σ","ΣY"],"route":["MATH","↓","ZOOM","↓","GRAPH","ZOOM"],"section":"STAT"},
    {"id":"STAT_REG_Y2","label":"ΣY2","path":["HOME","STAT","Σ","ΣY2"],"route":["MATH","↓","ZOOM","↓","GRAPH","TRACE"],"section":"STAT"},
    {"id":"STAT_REG_XY","label":"ΣXY","path":["HOME","STAT","Σ","ΣXY"],"route":["MATH","↓","ZOOM","↓","GRAPH","GRAPH"],"section":"STAT"},
    {"id":"STAT_REG_N","label":"ΣN","path":["HOME","STAT","Σ","ΣN"],"route":["MATH","↓","ZOOM","↓","GRAPH","↓","Y="],"section":"STAT"},
    {"id":"STAT_REG_LN_X","label":"ΣLX","path":["HOME","STAT","Σ","ΣLX"],"route":["MATH","↓","ZOOM","↓","GRAPH","↓","WINDOW"],"section":"STAT"},
    {"id":"STAT_REG_LN_X2","label":"ΣLX2","path":["HOME","STAT","Σ","ΣLX2"],"route":["MATH","↓","ZOOM","↓","GRAPH","↓","ZOOM"],"section":"STAT"},
    {"id":"STAT_REG_LN_Y","label":"ΣLY","path":["HOME","STAT","Σ","ΣLY"],"route":["MATH","↓","ZOOM","↓","GRAPH","↓","TRACE"],"section":"STAT"},
    {"id":"STAT_REG_LN_Y2","label":"ΣLY2","path":["HOME","STAT","Σ","ΣLY2"],"route":["MATH","↓","ZOOM","↓","GRAPH","↓","GRAPH"],"section":"STAT"},
    {"id":"STAT_REG_LN_XLN_Y","label":"ΣLXL","path":["HOME","STAT","Σ","ΣLXL"],"route":["MATH","↓","ZOOM","↓","GRAPH","↓","↓","Y="],"section":"STAT"},
    {"id":"STAT_REG_XLN_Y","label":"ΣXLY","path":["HOME","STAT","Σ","ΣXLY"],"route":["MATH","↓","ZOOM","↓","GRAPH","↓","↓","WINDOW"],"section":"STAT"},
    {"id":"STAT_REG_YLN_X","label":"ΣYLX","path":["HOME","STAT","Σ","ΣYLX"],"route":["MATH","↓","ZOOM","↓","GRAPH","↓","↓","ZOOM"],"section":"STAT"},
    {"id":"STAT_SAMPLE_SDEV","label":"SDEV","path":["HOME","STAT","SDEV"],"route":["MATH","↓","ZOOM","↓","↓","Y="],"section":"STAT"},
    {"id":"STAT_SAMPLE_COV","label":"SCOV","path":["HOME","STAT","SCOV"],"route":["MATH","↓","ZOOM","↓","↓","WINDOW"],"section":"STAT"},
    {"id":"STAT_POP_SDEV","label":"PDEV","path":["HOME","STAT","PDEV"],"route":["MATH","↓","ZOOM","↓","↓","ZOOM"],"section":"STAT"},
    {"id":"STAT_POP_COV","label":"PCOV","path":["HOME","STAT","PCOV"],"route":["MATH","↓","ZOOM","↓","↓","TRACE"],"section":"STAT"},
    {"id":"CFIT_FORECAST_X","label":"Y→X","path":["HOME","STAT","CFIT","Y→X"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","Y="],"section":"STAT"},
    {"id":"CFIT_FORECAST_Y","label":"X→Y","path":["HOME","STAT","CFIT","X→Y"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","WINDOW"],"section":"STAT"},
    {"id":"CFIT_SLOPE","label":"SLOP","path":["HOME","STAT","CFIT","SLOP"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","ZOOM"],"section":"STAT"},
    {"id":"CFIT_INTERCEPT","label":"YINT","path":["HOME","STAT","CFIT","YINT"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","TRACE"],"section":"STAT"},
    {"id":"CFIT_CORRELATION","label":"CORR","path":["HOME","STAT","CFIT","CORR"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","GRAPH"],"section":"STAT"},
    {"id":"CFIT_LINEAR","label":"LINF","path":["HOME","STAT","CFIT","LINF"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","↓","Y="],"section":"STAT"},
    {"id":"CFIT_LOG","label":"LOGF","path":["HOME","STAT","CFIT","LOGF"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","↓","WINDOW"],"section":"STAT"},
    {"id":"CFIT_EXP","label":"EXPF","path":["HOME","STAT","CFIT","EXPF"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","↓","ZOOM"],"section":"STAT"},
    {"id":"CFIT_POWER","label":"PWRF","path":["HOME","STAT","CFIT","PWRF"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","↓","TRACE"],"section":"STAT"},
    {"id":"CFIT_BEST","label":"BEST","path":["HOME","STAT","CFIT","BEST"],"route":["MATH","↓","ZOOM","↓","↓","GRAPH","↓","GRAPH"],"section":"STAT"},
    {"id":"RAD_TO_DEG","label":"→DEG","path":["HOME","CONV","→DEG"],"route":["MATH","↓","TRACE","Y="],"section":"CONV"},
    {"id":"DEG_TO_RAD","label":"→RAD","path":["HOME","CONV","→RAD"],"route":["MATH","↓","TRACE","WINDOW"],"section":"CONV"},
    {"id":"POLAR_TO_RECT","label":"→REC","path":["HOME","CONV","→REC"],"route":["MATH","↓","TRACE","TRACE"],"section":"CONV"},
    {"id":"RECT_TO_POLAR","label":"→POL","path":["HOME","CONV","→POL"],"route":["MATH","↓","TRACE","GRAPH"],"section":"CONV"},
    {"id":"HMS_TO_HR","label":"→HR","path":["HOME","CONV","→HR"],"route":["MATH","↓","TRACE","↓","Y="],"section":"CONV"},
    {"id":"HR_TO_HMS","label":"→HMS","path":["HOME","CONV","→HMS"],"route":["MATH","↓","TRACE","↓","WINDOW"],"section":"CONV"},
    {"id":"HMS_PLUS","label":"HMS+","path":["HOME","CONV","HMS+"],"route":["MATH","↓","TRACE","↓","ZOOM"],"section":"CONV"},
    {"id":"HMS_MINUS","label":"HMS-","path":["HOME","CONV","HMS-"],"route":["MATH","↓","TRACE","↓","TRACE"],"section":"CONV"},
    {"id":"HELP","label":"HELP","path":["HOME","HELP"],"route":["MATH","↓","GRAPH"],"section":"HELP"},
    {"id":"CLEAR_X","label":"CLX","path":["HOME","CLR","CLX"],"route":["MATH","↓","↓","Y=","Y="],"section":"CLR"},
    {"id":"CLEAR_STACK","label":"CLST","path":["HOME","CLR","CLST"],"route":["MATH","↓","↓","Y=","WINDOW"],"section":"CLR"},
    {"id":"CLEAR_REGS","label":"CLRG","path":["HOME","CLR","CLRG"],"route":["MATH","↓","↓","Y=","ZOOM"],"section":"CLR"},
    {"id":"CLEAR_STAT","label":"CLΣ","path":["HOME","CLR","CLΣ"],"route":["MATH","↓","↓","Y=","TRACE"],"section":"CLR"},
    {"id":"CLEAR_TVM","label":"CLTV","path":["HOME","CLR","CLTV"],"route":["MATH","↓","↓","Y=","GRAPH"],"section":"CLR"},
    {"id":"CLEAR_DISPLAY","label":"CLD","path":["HOME","CLR","CLD"],"route":["MATH","↓","↓","Y=","↓","Y="],"section":"CLR"},
    {"id":"FIX","label":"FIX","path":["HOME","MODE","FIX"],"route":["MATH","↓","↓","WINDOW","Y="],"section":"MODE"},
    {"id":"SCI","label":"SCI","path":["HOME","MODE","SCI"],"route":["MATH","↓","↓","WINDOW","WINDOW"],"section":"MODE"},
    {"id":"ENG","label":"ENG","path":["HOME","MODE","ENG"],"route":["MATH","↓","↓","WINDOW","ZOOM"],"section":"MODE"},
    {"id":"RAD","label":"RAD","path":["HOME","MODE","RAD"],"route":["MATH","↓","↓","WINDOW","TRACE"],"section":"MODE"},
    {"id":"DEG","label":"DEG","path":["HOME","MODE","DEG"],"route":["MATH","↓","↓","WINDOW","GRAPH"],"section":"MODE"},
    {"id":"NUM_RESULT_MODE_REAL","label":"RRES","path":["HOME","MODE","RRES"],"route":["MATH","↓","↓","WINDOW","↓","Y="],"section":"MODE"},
    {"id":"NUM_RESULT_MODE_COMPLEX","label":"CRES","path":["HOME","MODE","CRES"],"route":["MATH","↓","↓","WINDOW","↓","WINDOW"],"section":"MODE"},
    {"id":"COMPLEX_MODE_RECT","label":"RECT","path":["HOME","MODE","RECT"],"route":["MATH","↓","↓","WINDOW","↓","ZOOM"],"section":"MODE"},
    {"id":"COMPLEX_MODE_RAD","label":"PRAD","path":["HOME","MODE","PRAD"],"route":["MATH","↓","↓","WINDOW","↓","TRACE"],"section":"MODE"},
    {"id":"COMPLEX_MODE_DEG","label":"PDEG","path":["HOME","MODE","PDEG"],"route":["MATH","↓","↓","WINDOW","↓","GRAPH"],"section":"MODE"},
    {"id":"SET_REG_SIZE","label":"RSIZ","path":["HOME","MODE","RSIZ"],"route":["MATH","↓","↓","WINDOW","↓","↓","Y="],"section":"MODE"},
    {"id":"GET_REG_SIZE","label":"RSZ?","path":["HOME","MODE","RSZ?"],"route":["MATH","↓","↓","WINDOW","↓","↓","WINDOW"],"section":"MODE"},
    {"id":"SCREEN_TINT","label":"TINT","path":["HOME","MODE","TINT"],"route":["MATH","↓","↓","WINDOW","↓","↓","ZOOM"],"section":"MODE"},
    {"id":"SET_STACK_SIZE","label":"SSIZ","path":["HOME","MODE","SSIZ"],"route":["MATH","↓","↓","WINDOW","↓","↓","TRACE"],"section":"MODE"},
    {"id":"GET_STACK_SIZE","label":"SSZ?","path":["HOME","MODE","SSZ?"],"route":["MATH","↓","↓","WINDOW","↓","↓","GRAPH"],"section":"MODE"},
    {"id":"FORMAT_RECORD_RAW","label":"{..}","path":["HOME","MODE","{..}"],"route":["MATH","↓","↓","WINDOW","↓","↓","↓","Y="],"section":"MODE"},
    {"id":"FORMAT_RECORD_STRING","label":"\"..\"","path":["HOME","MODE","\"..\""],"route":["MATH","↓","↓","WINDOW","↓","↓","↓","WINDOW"],"section":"MODE"},
    {"id":"DECIMAL_POINT","label":".","path":["HOME","MODE","."],"route":["MATH","↓","↓","WINDOW","↓","↓","↓","ZOOM"],"section":"MODE"},
    {"id":"DECIMAL_COMMA","label":",","path":["HOME","MODE",","],"route":["MATH","↓","↓","WINDOW","↓","↓","↓","TRACE"],"section":"MODE"},
    {"id":"STACK_DUP","label":"DUP","path":["HOME","STK","DUP"],"route":["MATH","↓","↓","ZOOM","Y="],"section":"STK"},
    {"id":"STACK_ROLL_UP","label":"R↑","path":["HOME","STK","R↑"],"route":["MATH","↓","↓","ZOOM","WINDOW"],"section":"STK"},
    {"id":"STACK_ROLL_DOWN","label":"R↓","path":["HOME","STK","R↓"],"route":["MATH","↓","↓","ZOOM","ZOOM"],"section":"STK"},
    {"id":"STACK_DROP","label":"DROP","path":["HOME","STK","DROP"],"route":["MATH","↓","↓","ZOOM","TRACE"],"section":"STK"},
    {"id":"STACK_EXCHANGE_XY","label":"X←→Y","path":["HOME","STK","X←→Y"],"route":["MATH","↓","↓","ZOOM","GRAPH"],"section":"STK"},
    {"id":"STACK_EXCHANGE_REG","label":"X<>","path":["HOME","STK","X<>"],"route":["MATH","↓","↓","ZOOM","↓","Y="],"section":"STK"},
    {"id":"UNIT_VALUE","label":"UVAL","path":["HOME","UNIT","UFCN","UVAL"],"route":["MATH","↓","↓","TRACE","Y=","Y="],"section":"UNIT"},
    {"id":"UNIT_BASE","label":"UBAS","path":["HOME","UNIT","UFCN","UBAS"],"route":["MATH","↓","↓","TRACE","Y=","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_FERMI","label":"ferm","path":["HOME","UNIT","LENG","ferm"],"route":["MATH","↓","↓","TRACE","WINDOW","Y="],"section":"UNIT"},
    {"id":"UNIT_ANGSTROM","label":"angs","path":["HOME","UNIT","LENG","angs"],"route":["MATH","↓","↓","TRACE","WINDOW","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_NANO_METER","label":"nm","path":["HOME","UNIT","LENG","nm"],"route":["MATH","↓","↓","TRACE","WINDOW","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_MICRO_METER","label":"µm","path":["HOME","UNIT","LENG","µm"],"route":["MATH","↓","↓","TRACE","WINDOW","TRACE"],"section":"UNIT"},
    {"id":"UNIT_MILLI_METER","label":"mm","path":["HOME","UNIT","LENG","mm"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_CENTI_METER","label":"cm","path":["HOME","UNIT","LENG","cm"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_METER","label":"m","path":["HOME","UNIT","LENG","m"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_KILO_METER","label":"km","path":["HOME","UNIT","LENG","km"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_MIL","label":"mil","path":["HOME","UNIT","LENG","mil"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_INCH","label":"in","path":["HOME","UNIT","LENG","in"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_FOOT","label":"ft","path":["HOME","UNIT","LENG","ft"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_YARD","label":"yd","path":["HOME","UNIT","LENG","yd"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_MILE","label":"mi","path":["HOME","UNIT","LENG","mi"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_TWIP","label":"twip","path":["HOME","UNIT","LENG","TYPO","twip"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","Y=","Y="],"section":"UNIT"},
    {"id":"UNIT_POINT","label":"pt","path":["HOME","UNIT","LENG","TYPO","pt"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","Y=","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_PICA","label":"pica","path":["HOME","UNIT","LENG","TYPO","pica"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","Y=","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_SURVEY_FOOT","label":"svft","path":["HOME","UNIT","LENG","SURV","svft"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","WINDOW","Y="],"section":"UNIT"},
    {"id":"UNIT_ROD","label":"rod","path":["HOME","UNIT","LENG","SURV","rod"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","WINDOW","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_CHAIN","label":"chai","path":["HOME","UNIT","LENG","SURV","chai"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","WINDOW","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_FURLONG","label":"frlg","path":["HOME","UNIT","LENG","SURV","frlg"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","WINDOW","TRACE"],"section":"UNIT"},
    {"id":"UNIT_SURVEY_MILE","label":"svmi","path":["HOME","UNIT","LENG","SURV","svmi"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","WINDOW","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_LEAGUE","label":"leag","path":["HOME","UNIT","LENG","SURV","leag"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","WINDOW","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_FATHOM","label":"fath","path":["HOME","UNIT","LENG","NAUT","fath"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","ZOOM","Y="],"section":"UNIT"},
    {"id":"UNIT_CABLE","label":"cabl","path":["HOME","UNIT","LENG","NAUT","cabl"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","ZOOM","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_NAUTICAL_MILE","label":"nmi","path":["HOME","UNIT","LENG","NAUT","nmi"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","ZOOM","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_LIGHT_SECOND","label":"lsec","path":["HOME","UNIT","LENG","ASTR","lsec"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","TRACE","Y="],"section":"UNIT"},
    {"id":"UNIT_ASTRONOMICAL_UNIT","label":"AU","path":["HOME","UNIT","LENG","ASTR","AU"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","TRACE","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_LIGHT_YEAR","label":"ly","path":["HOME","UNIT","LENG","ASTR","ly"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","TRACE","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_PARSEC","label":"pc","path":["HOME","UNIT","LENG","ASTR","pc"],"route":["MATH","↓","↓","TRACE","WINDOW","↓","↓","↓","TRACE","TRACE"],"section":"UNIT"},
    {"id":"UNIT_SQ_MILLI_METER","label":"mm²","path":["HOME","UNIT","AREA","mm²"],"route":["MATH","↓","↓","TRACE","ZOOM","Y="],"section":"UNIT"},
    {"id":"UNIT_SQ_CENTI_METER","label":"cm²","path":["HOME","UNIT","AREA","cm²"],"route":["MATH","↓","↓","TRACE","ZOOM","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_SQ_METER","label":"m²","path":["HOME","UNIT","AREA","m²"],"route":["MATH","↓","↓","TRACE","ZOOM","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_SQ_KILO_METER","label":"km²","path":["HOME","UNIT","AREA","km²"],"route":["MATH","↓","↓","TRACE","ZOOM","TRACE"],"section":"UNIT"},
    {"id":"UNIT_SQ_INCH","label":"in²","path":["HOME","UNIT","AREA","in²"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_SQ_FOOT","label":"ft²","path":["HOME","UNIT","AREA","ft²"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_SQ_YARD","label":"yd²","path":["HOME","UNIT","AREA","yd²"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_SQ_MILE","label":"mi²","path":["HOME","UNIT","AREA","mi²"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_SQ_NAUTICAL_MILE","label":"nmi²","path":["HOME","UNIT","AREA","nmi²"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_ACRE","label":"acre","path":["HOME","UNIT","AREA","acre"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_HECTARE","label":"ha","path":["HOME","UNIT","AREA","ha"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_USFOOTBALL","label":"usfb","path":["HOME","UNIT","AREA","usfb"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_CAFOOTBALL","label":"cafb","path":["HOME","UNIT","AREA","cafb"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_SQ_ROD","label":"rod²","path":["HOME","UNIT","AREA","SURV","rod²"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","↓","GRAPH","Y="],"section":"UNIT"},
    {"id":"UNIT_SQ_CHAIN","label":"chn²","path":["HOME","UNIT","AREA","SURV","chn²"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","↓","GRAPH","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_SQ_FURLONG","label":"fur²","path":["HOME","UNIT","AREA","SURV","fur²"],"route":["MATH","↓","↓","TRACE","ZOOM","↓","↓","GRAPH","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_CU_MILLI_METER","label":"mm³","path":["HOME","UNIT","VOL","mm³"],"route":["MATH","↓","↓","TRACE","TRACE","Y="],"section":"UNIT"},
    {"id":"UNIT_CU_CENTI_METER","label":"cm³","path":["HOME","UNIT","VOL","cm³"],"route":["MATH","↓","↓","TRACE","TRACE","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_CU_METER","label":"m³","path":["HOME","UNIT","VOL","m³"],"route":["MATH","↓","↓","TRACE","TRACE","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_CU_KILO_METER","label":"km³","path":["HOME","UNIT","VOL","km³"],"route":["MATH","↓","↓","TRACE","TRACE","TRACE"],"section":"UNIT"},
    {"id":"UNIT_CU_INCH","label":"in³","path":["HOME","UNIT","VOL","in³"],"route":["MATH","↓","↓","TRACE","TRACE","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_CU_FOOT","label":"ft³","path":["HOME","UNIT","VOL","ft³"],"route":["MATH","↓","↓","TRACE","TRACE","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_CU_YARD","label":"yd³","path":["HOME","UNIT","VOL","yd³"],"route":["MATH","↓","↓","TRACE","TRACE","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_CU_MILE","label":"mi³","path":["HOME","UNIT","VOL","mi³"],"route":["MATH","↓","↓","TRACE","TRACE","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_CU_NAUTICAL_MILE","label":"nmi³","path":["HOME","UNIT","VOL","nmi³"],"route":["MATH","↓","↓","TRACE","TRACE","↓","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_MICRO_LITER","label":"µL","path":["HOME","UNIT","VOL","µL"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_MILLI_LITER","label":"mL","path":["HOME","UNIT","VOL","mL"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_LITER","label":"L","path":["HOME","UNIT","VOL","L"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_METRIC_TEASPOON","label":"mtsp","path":["HOME","UNIT","VOL","mtsp"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_METRIC_TABLESPOON","label":"mtbs","path":["HOME","UNIT","VOL","mtbs"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_TEASPOON","label":"tsp","path":["HOME","UNIT","VOL","US","tsp"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","Y=","Y="],"section":"UNIT"},
    {"id":"UNIT_TABLESPOON","label":"tbsp","path":["HOME","UNIT","VOL","US","tbsp"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","Y=","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_FLUID_OUNCE","label":"floz","path":["HOME","UNIT","VOL","US","floz"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","Y=","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_GILL","label":"gill","path":["HOME","UNIT","VOL","US","gill"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","Y=","TRACE"],"section":"UNIT"},
    {"id":"UNIT_CUP","label":"cup","path":["HOME","UNIT","VOL","US","cup"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","Y=","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_PINT","label":"pint","path":["HOME","UNIT","VOL","US","pint"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","Y=","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_QUART","label":"qt","path":["HOME","UNIT","VOL","US","qt"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","Y=","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_GALLON","label":"gal","path":["HOME","UNIT","VOL","US","gal"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","Y=","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_IMP_TEASPOON","label":"tsp","path":["HOME","UNIT","VOL","IMP","tsp"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","WINDOW","Y="],"section":"UNIT"},
    {"id":"UNIT_IMP_TABLESPOON","label":"tbsp","path":["HOME","UNIT","VOL","IMP","tbsp"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","WINDOW","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_IMP_FLUID_OUNCE","label":"floz","path":["HOME","UNIT","VOL","IMP","floz"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","WINDOW","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_IMP_GILL","label":"gill","path":["HOME","UNIT","VOL","IMP","gill"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","WINDOW","TRACE"],"section":"UNIT"},
    {"id":"UNIT_IMP_CUP","label":"cup","path":["HOME","UNIT","VOL","IMP","cup"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","WINDOW","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_IMP_PINT","label":"pint","path":["HOME","UNIT","VOL","IMP","pint"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","WINDOW","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_IMP_QUART","label":"qt","path":["HOME","UNIT","VOL","IMP","qt"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","WINDOW","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_IMP_GALLON","label":"gal","path":["HOME","UNIT","VOL","IMP","gal"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","WINDOW","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_DRY_PINT","label":"drpt","path":["HOME","UNIT","VOL","DRY","drpt"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","ZOOM","Y="],"section":"UNIT"},
    {"id":"UNIT_DRY_QUART","label":"drqt","path":["HOME","UNIT","VOL","DRY","drqt"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","ZOOM","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_DRY_GALLON","label":"dgal","path":["HOME","UNIT","VOL","DRY","dgal"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","ZOOM","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_PECK","label":"peck","path":["HOME","UNIT","VOL","DRY","peck"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","ZOOM","TRACE"],"section":"UNIT"},
    {"id":"UNIT_BUSHEL","label":"bush","path":["HOME","UNIT","VOL","DRY","bush"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","ZOOM","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_DRY_BARREL","label":"dbbl","path":["HOME","UNIT","VOL","DRY","dbbl"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","ZOOM","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_BOARD_FOOT","label":"bdft","path":["HOME","UNIT","VOL","MISC","bdft"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","TRACE","Y="],"section":"UNIT"},
    {"id":"UNIT_OIL_BARREL","label":"bbl","path":["HOME","UNIT","VOL","MISC","bbl"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","TRACE","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_OLYMPIC_POOL","label":"olmp","path":["HOME","UNIT","VOL","MISC","olmp"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","TRACE","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_ACRE_FOOT","label":"acft","path":["HOME","UNIT","VOL","MISC","acft"],"route":["MATH","↓","↓","TRACE","TRACE","↓","↓","↓","TRACE","TRACE"],"section":"UNIT"},
    {"id":"UNIT_CELSIUS","label":"°C","path":["HOME","UNIT","TEMP","°C"],"route":["MATH","↓","↓","TRACE","GRAPH","Y="],"section":"UNIT"},
    {"id":"UNIT_FAHRENHEIT","label":"°F","path":["HOME","UNIT","TEMP","°F"],"route":["MATH","↓","↓","TRACE","GRAPH","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_RANKINE","label":"°R","path":["HOME","UNIT","TEMP","°R"],"route":["MATH","↓","↓","TRACE","GRAPH","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_KELVIN","label":"°K","path":["HOME","UNIT","TEMP","°K"],"route":["MATH","↓","↓","TRACE","GRAPH","TRACE"],"section":"UNIT"},
    {"id":"UNIT_MICRO_GRAM","label":"µg","path":["HOME","UNIT","MASS","µg"],"route":["MATH","↓","↓","TRACE","↓","Y=","Y="],"section":"UNIT"},
    {"id":"UNIT_MILLI_GRAM","label":"mg","path":["HOME","UNIT","MASS","mg"],"route":["MATH","↓","↓","TRACE","↓","Y=","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_GRAM","label":"g","path":["HOME","UNIT","MASS","g"],"route":["MATH","↓","↓","TRACE","↓","Y=","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_KILO_GRAM","label":"kg","path":["HOME","UNIT","MASS","kg"],"route":["MATH","↓","↓","TRACE","↓","Y=","TRACE"],"section":"UNIT"},
    {"id":"UNIT_METRIC_TON","label":"t","path":["HOME","UNIT","MASS","t"],"route":["MATH","↓","↓","TRACE","↓","Y=","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_ATOMIC_MASS_UNIT","label":"amu","path":["HOME","UNIT","MASS","amu"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_GRAIN","label":"grai","path":["HOME","UNIT","MASS","US","grai"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","WINDOW","Y="],"section":"UNIT"},
    {"id":"UNIT_DRAM","label":"dram","path":["HOME","UNIT","MASS","US","dram"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","WINDOW","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_OUNCE","label":"oz","path":["HOME","UNIT","MASS","US","oz"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","WINDOW","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_POUND","label":"lb","path":["HOME","UNIT","MASS","US","lb"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","WINDOW","TRACE"],"section":"UNIT"},
    {"id":"UNIT_SLUG","label":"slug","path":["HOME","UNIT","MASS","US","slug"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","WINDOW","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_SHORT_HUNDRED_WEIGHT","label":"scwt","path":["HOME","UNIT","MASS","US","scwt"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","WINDOW","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_SHORT_TON","label":"ston","path":["HOME","UNIT","MASS","US","ston"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","WINDOW","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_IMP_POUND","label":"lb","path":["HOME","UNIT","MASS","IMP","lb"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","ZOOM","Y="],"section":"UNIT"},
    {"id":"UNIT_STONE","label":"stne","path":["HOME","UNIT","MASS","IMP","stne"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","ZOOM","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_QUARTER","label":"qrtr","path":["HOME","UNIT","MASS","IMP","qrtr"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","ZOOM","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_LONG_HUNDRED_WEIGHT","label":"lcwt","path":["HOME","UNIT","MASS","IMP","lcwt"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","ZOOM","TRACE"],"section":"UNIT"},
    {"id":"UNIT_LONG_TON","label":"lton","path":["HOME","UNIT","MASS","IMP","lton"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","ZOOM","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_TROY_GRAIN","label":"grai","path":["HOME","UNIT","MASS","TROY","grai"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","TRACE","Y="],"section":"UNIT"},
    {"id":"UNIT_TROY_PENNY_WEIGHT","label":"dwt","path":["HOME","UNIT","MASS","TROY","dwt"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","TRACE","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_TROY_OUNCE","label":"ozt","path":["HOME","UNIT","MASS","TROY","ozt"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","TRACE","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_TROY_POUND","label":"lbt","path":["HOME","UNIT","MASS","TROY","lbt"],"route":["MATH","↓","↓","TRACE","↓","Y=","↓","TRACE","TRACE"],"section":"UNIT"},
    {"id":"UNIT_DYNE","label":"dyne","path":["HOME","UNIT","FORC","dyne"],"route":["MATH","↓","↓","TRACE","↓","WINDOW","Y="],"section":"UNIT"},
    {"id":"UNIT_NEWTON","label":"N","path":["HOME","UNIT","FORC","N"],"route":["MATH","↓","↓","TRACE","↓","WINDOW","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_KILOGRAM_FORCE","label":"kgf","path":["HOME","UNIT","FORC","kgf"],"route":["MATH","↓","↓","TRACE","↓","WINDOW","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_METRIC_TON_FORCE","label":"tonf","path":["HOME","UNIT","FORC","tonf"],"route":["MATH","↓","↓","TRACE","↓","WINDOW","TRACE"],"section":"UNIT"},
    {"id":"UNIT_POUNDAL","label":"pdl","path":["HOME","UNIT","FORC","pdl"],"route":["MATH","↓","↓","TRACE","↓","WINDOW","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_POUND_FORCE","label":"lbf","path":["HOME","UNIT","FORC","lbf"],"route":["MATH","↓","↓","TRACE","↓","WINDOW","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_SHORT_TON_FORCE","label":"stnf","path":["HOME","UNIT","FORC","stnf"],"route":["MATH","↓","↓","TRACE","↓","WINDOW","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_LONG_TON_FORCE","label":"ltnf","path":["HOME","UNIT","FORC","ltnf"],"route":["MATH","↓","↓","TRACE","↓","WINDOW","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_PASCAL","label":"Pa","path":["HOME","UNIT","PRES","Pa"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","Y="],"section":"UNIT"},
    {"id":"UNIT_HECTO_PASCAL","label":"hPa","path":["HOME","UNIT","PRES","hPa"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_KILO_PASCAL","label":"kPa","path":["HOME","UNIT","PRES","kPa"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_TORR","label":"torr","path":["HOME","UNIT","PRES","torr"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","TRACE"],"section":"UNIT"},
    {"id":"UNIT_ATMOSPHERE","label":"atm","path":["HOME","UNIT","PRES","atm"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_MILLI_BAR","label":"mbar","path":["HOME","UNIT","PRES","mbar"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_DECI_BAR","label":"dbar","path":["HOME","UNIT","PRES","dbar"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_BAR","label":"bar","path":["HOME","UNIT","PRES","bar"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_POUND_SQUARE_INCH","label":"psi","path":["HOME","UNIT","PRES","psi"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_MILLI_METER_MERCURY","label":"mmH","path":["HOME","UNIT","PRES","mmH"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","↓","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_INCH_MERCURY","label":"inHg","path":["HOME","UNIT","PRES","inHg"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","↓","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_MILLI_METER_WATER","label":"mmw","path":["HOME","UNIT","PRES","mmw"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","↓","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_INCH_WATER","label":"inwg","path":["HOME","UNIT","PRES","inwg"],"route":["MATH","↓","↓","TRACE","↓","ZOOM","↓","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_ELECTRON_VOLT","label":"eV","path":["HOME","UNIT","ENER","eV"],"route":["MATH","↓","↓","TRACE","↓","TRACE","Y="],"section":"UNIT"},
    {"id":"UNIT_ERG","label":"erg","path":["HOME","UNIT","ENER","erg"],"route":["MATH","↓","↓","TRACE","↓","TRACE","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_JOULE","label":"J","path":["HOME","UNIT","ENER","J"],"route":["MATH","↓","↓","TRACE","↓","TRACE","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_KILO_JOULE","label":"kJ","path":["HOME","UNIT","ENER","kJ"],"route":["MATH","↓","↓","TRACE","↓","TRACE","TRACE"],"section":"UNIT"},
    {"id":"UNIT_FOOT_POUND_ENERGY","label":"ftlb","path":["HOME","UNIT","ENER","ftlb"],"route":["MATH","↓","↓","TRACE","↓","TRACE","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_WATT_HOUR","label":"Wh","path":["HOME","UNIT","ENER","Wh"],"route":["MATH","↓","↓","TRACE","↓","TRACE","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_KILO_WATT_HOUR","label":"kWh","path":["HOME","UNIT","ENER","kWh"],"route":["MATH","↓","↓","TRACE","↓","TRACE","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_CALORIE","label":"cal","path":["HOME","UNIT","ENER","cal"],"route":["MATH","↓","↓","TRACE","↓","TRACE","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_KILO_CALORIE","label":"kcal","path":["HOME","UNIT","ENER","kcal"],"route":["MATH","↓","↓","TRACE","↓","TRACE","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_BRITISH_THERMAL_UNIT","label":"Btu","path":["HOME","UNIT","ENER","Btu"],"route":["MATH","↓","↓","TRACE","↓","TRACE","↓","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_TON_TNT","label":"tTNT","path":["HOME","UNIT","ENER","tTNT"],"route":["MATH","↓","↓","TRACE","↓","TRACE","↓","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_LITER_ATMOSPHERE","label":"Latm","path":["HOME","UNIT","ENER","Latm"],"route":["MATH","↓","↓","TRACE","↓","TRACE","↓","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_WATT","label":"W","path":["HOME","UNIT","PWR","W"],"route":["MATH","↓","↓","TRACE","↓","GRAPH","Y="],"section":"UNIT"},
    {"id":"UNIT_KILO_WATT","label":"kW","path":["HOME","UNIT","PWR","kW"],"route":["MATH","↓","↓","TRACE","↓","GRAPH","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_FOOT_POUND_ENERGY_PER_SECOND","label":"fl/s","path":["HOME","UNIT","PWR","fl/s"],"route":["MATH","↓","↓","TRACE","↓","GRAPH","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_BTU_PER_HOUR","label":"Bt/h","path":["HOME","UNIT","PWR","Bt/h"],"route":["MATH","↓","↓","TRACE","↓","GRAPH","TRACE"],"section":"UNIT"},
    {"id":"UNIT_BTU_PER_MINUTE","label":"Bt/m","path":["HOME","UNIT","PWR","Bt/m"],"route":["MATH","↓","↓","TRACE","↓","GRAPH","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_CALORIE_PER_SECOND","label":"ca/s","path":["HOME","UNIT","PWR","ca/s"],"route":["MATH","↓","↓","TRACE","↓","GRAPH","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_HORSEPOWER","label":"hp","path":["HOME","UNIT","PWR","hp"],"route":["MATH","↓","↓","TRACE","↓","GRAPH","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_NANO_SECOND","label":"ns","path":["HOME","UNIT","TIME","ns"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","Y="],"section":"UNIT"},
    {"id":"UNIT_MICRO_SECOND","label":"µs","path":["HOME","UNIT","TIME","µs"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_MILLI_SECOND","label":"ms","path":["HOME","UNIT","TIME","ms"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_SECOND","label":"sec","path":["HOME","UNIT","TIME","sec"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","TRACE"],"section":"UNIT"},
    {"id":"UNIT_MINUTE","label":"min","path":["HOME","UNIT","TIME","min"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_HOUR","label":"hour","path":["HOME","UNIT","TIME","hour"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_DAY","label":"day","path":["HOME","UNIT","TIME","day"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","↓","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_WEEK","label":"week","path":["HOME","UNIT","TIME","week"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","↓","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_JULIAN_MONTH","label":"mon","path":["HOME","UNIT","TIME","mon"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","↓","TRACE"],"section":"UNIT"},
    {"id":"UNIT_JULIAN_YEAR","label":"year","path":["HOME","UNIT","TIME","year"],"route":["MATH","↓","↓","TRACE","↓","↓","Y=","↓","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_METER_PER_SECOND","label":"m/s","path":["HOME","UNIT","SPD","m/s"],"route":["MATH","↓","↓","TRACE","↓","↓","WINDOW","Y="],"section":"UNIT"},
    {"id":"UNIT_FOOT_PER_SECOND","label":"ft/s","path":["HOME","UNIT","SPD","ft/s"],"route":["MATH","↓","↓","TRACE","↓","↓","WINDOW","WINDOW"],"section":"UNIT"},
    {"id":"UNIT_KILO_METER_PER_HOUR","label":"kph","path":["HOME","UNIT","SPD","kph"],"route":["MATH","↓","↓","TRACE","↓","↓","WINDOW","ZOOM"],"section":"UNIT"},
    {"id":"UNIT_MILE_PER_HOUR","label":"mph","path":["HOME","UNIT","SPD","mph"],"route":["MATH","↓","↓","TRACE","↓","↓","WINDOW","TRACE"],"section":"UNIT"},
    {"id":"UNIT_KNOT","label":"knot","path":["HOME","UNIT","SPD","knot"],"route":["MATH","↓","↓","TRACE","↓","↓","WINDOW","GRAPH"],"section":"UNIT"},
    {"id":"UNIT_LIGHT_SPEED","label":"c","path":["HOME","UNIT","SPD","c"],"route":["MATH","↓","↓","TRACE","↓","↓","WINDOW","↓","Y="],"section":"UNIT"},
    {"id":"UNIT_LITERS_PER_HUNDRED_KILO_METERS","label":"Lkm","path":["HOME","UNIT","FUEL","Lkm"],"route":["MATH","↓","↓","TRACE","↓","↓","ZOOM","Y="],"section":"UNIT"},
    {"id":"UNIT_MILES_PER_GALLON","label":"mpg","path":["HOME","UNIT","FUEL","mpg"],"route":["MATH","↓","↓","TRACE","↓","↓","ZOOM","WINDOW"],"section":"UNIT"},
    {"id":"DATE_CREATE","label":"D{}","path":["HOME","DATE","D","D{}"],"route":["MATH","↓","↓","GRAPH","Y=","Y="],"section":"DATE"},
    {"id":"DATE_TO_EPOCH_DAYS","label":"→ED","path":["HOME","DATE","D","→ED"],"route":["MATH","↓","↓","GRAPH","Y=","WINDOW"],"section":"DATE"},
    {"id":"EPOCH_DAYS_TO_DATE","label":"ED→","path":["HOME","DATE","D","ED→"],"route":["MATH","↓","↓","GRAPH","Y=","ZOOM"],"section":"DATE"},
    {"id":"DATE_TO_EPOCH_SECONDS","label":"→ES","path":["HOME","DATE","D","→ES"],"route":["MATH","↓","↓","GRAPH","Y=","TRACE"],"section":"DATE"},
    {"id":"EPOCH_SECONDS_TO_DATE","label":"ES→","path":["HOME","DATE","D","ES→"],"route":["MATH","↓","↓","GRAPH","Y=","GRAPH"],"section":"DATE"},
    {"id":"IS_DATE_LEAP","label":"LEAP","path":["HOME","DATE","D","LEAP"],"route":["MATH","↓","↓","GRAPH","Y=","↓","Y="],"section":"DATE"},
    {"id":"DATE_TO_DAY_OF_WEEK","label":"DOW","path":["HOME","DATE","D","DOW"],"route":["MATH","↓","↓","GRAPH","Y=","↓","WINDOW"],"section":"DATE"},
    {"id":"DATE_CONVERT_TO_TIME_ZONE","label":"CVTZ","path":["HOME","DATE","D","CVTZ"],"route":["MATH","↓","↓","GRAPH","Y=","↓","ZOOM"],"section":"DATE"},
    {"id":"DATE_EXTRACT_YEAR","label":".YR","path":["HOME","DATE","D",".YR"],"route":["MATH","↓","↓","GRAPH","Y=","↓","↓","Y="],"section":"DATE"},
    {"id":"DATE_EXTRACT_MONTH","label":".MON","path":["HOME","DATE","D",".MON"],"route":["MATH","↓","↓","GRAPH","Y=","↓","↓","WINDOW"],"section":"DATE"},
    {"id":"DATE_EXTRACT_DAY","label":".DAY","path":["HOME","DATE","D",".DAY"],"route":["MATH","↓","↓","GRAPH","Y=","↓","↓","ZOOM"],"section":"DATE"},
    {"id":"DATE_SHRINK_TO_NOTHING","label":"DSHK","path":["HOME","DATE","D","DSHK"],"route":["MATH","↓","↓","GRAPH","Y=","↓","↓","↓","Y="],"section":"DATE"},
    {"id":"DATE_EXTEND_TO_DATE_TIME","label":"DEXD","path":["HOME","DATE","D","DEXD"],"route":["MATH","↓","↓","GRAPH","Y=","↓","↓","↓","WINDOW"],"section":"DATE"},
    {"id":"DATE_CUT_TO_NOTHING","label":"DCUT","path":["HOME","DATE","D","DCUT"],"route":["MATH","↓","↓","GRAPH","Y=","↓","↓","↓","ZOOM"],"section":"DATE"},
    {"id":"DATE_LINK_TO_DATE_TIME","label":"DLNK","path":["HOME","DATE","D","DLNK"],"route":["MATH","↓","↓","GRAPH","Y=","↓","↓","↓","TRACE"],"section":"DATE"},
    {"id":"TIME_CREATE","label":"T{}","path":["HOME","DATE","T","T{}"],"route":["MATH","↓","↓","GRAPH","WINDOW","Y="],"section":"DATE"},
    {"id":"TIME_TO_SECONDS","label":"→S","path":["HOME","DATE","T","→S"],"route":["MATH","↓","↓","GRAPH","WINDOW","WINDOW"],"section":"DATE"},
    {"id":"SECONDS_TO_TIME","label":"S→","path":["HOME","DATE","T","S→"],"route":["MATH","↓","↓","GRAPH","WINDOW","ZOOM"],"section":"DATE"},
    {"id":"TIME_EXTRACT_HOUR","label":".HR","path":["HOME","DATE","T",".HR"],"route":["MATH","↓","↓","GRAPH","WINDOW","↓","Y="],"section":"DATE"},
    {"id":"TIME_EXTRACT_MINUTE","label":".MIN","path":["HOME","DATE","T",".MIN"],"route":["MATH","↓","↓","GRAPH","WINDOW","↓","WINDOW"],"section":"DATE"},
    {"id":"TIME_EXTRACT_SECOND","label":".SEC","path":["HOME","DATE","T",".SEC"],"route":["MATH","↓","↓","GRAPH","WINDOW","↓","ZOOM"],"section":"DATE"},
    {"id":"DATE_TIME_CREATE","label":"DT{}","path":["HOME","DATE","DT","DT{}"],"route":["MATH","↓","↓","GRAPH","ZOOM","Y="],"section":"DATE"},
    {"id":"DATE_TIME_TO_EPOCH_DAYS","label":"→ED","path":["HOME","DATE","DT","→ED"],"route":["MATH","↓","↓","GRAPH","ZOOM","WINDOW"],"section":"DATE"},
    {"id":"EPOCH_DAYS_TO_DATE_TIME","label":"ED→","path":["HOME","DATE","DT","ED→"],"route":["MATH","↓","↓","GRAPH","ZOOM","ZOOM"],"section":"DATE"},
    {"id":"DATE_TIME_TO_EPOCH_SECONDS","label":"→ES","path":["HOME","DATE","DT","→ES"],"route":["MATH","↓","↓","GRAPH","ZOOM","TRACE"],"section":"DATE"},
    {"id":"EPOCH_SECONDS_TO_DATE_TIME","label":"ES→","path":["HOME","DATE","DT","ES→"],"route":["MATH","↓","↓","GRAPH","ZOOM","GRAPH"],"section":"DATE"},
    {"id":"IS_DATE_TIME_LEAP","label":"LEAP","path":["HOME","DATE","DT","LEAP"],"route":["MATH","↓","↓","GRAPH","ZOOM","↓","Y="],"section":"DATE"},
    {"id":"DATE_TIME_TO_DAY_OF_WEEK","label":"DOW","path":["HOME","DATE","DT","DOW"],"route":["MATH","↓","↓","GRAPH","ZOOM","↓","WINDOW"],"section":"DATE"},
    {"id":"DATE_TIME_CONVERT_TO_TIME_ZONE","label":"CVTZ","path":["HOME","DATE","DT","CVTZ"],"route":["MATH","↓","↓","GRAPH","ZOOM","↓","ZOOM"],"section":"DATE"},
    {"id":"DATE_TIME_EXTRACT_DATE","label":".D{}","path":["HOME","DATE","DT",".D{}"],"route":["MATH","↓","↓","GRAPH","ZOOM","↓","TRACE"],"section":"DATE"},
    {"id":"DATE_TIME_EXTRACT_TIME","label":".T{}","path":["HOME","DATE","DT",".T{}"],"route":["MATH","↓","↓","GRAPH","ZOOM","↓","GRAPH"],"section":"DATE"},
    {"id":"DATE_TIME_SHRINK_TO_DATE","label":"DSHK","path":["HOME","DATE","DT","DSHK"],"route":["MATH","↓","↓","GRAPH","ZOOM","↓","↓","Y="],"section":"DATE"},
    {"id":"DATE_TIME_EXTEND_TO_OFFSET_DATE_TIME","label":"DEXD","path":["HOME","DATE","DT","DEXD"],"route":["MATH","↓","↓","GRAPH","ZOOM","↓","↓","WINDOW"],"section":"DATE"},
    {"id":"DATE_TIME_CUT_TO_DATE","label":"DCUT","path":["HOME","DATE","DT","DCUT"],"route":["MATH","↓","↓","GRAPH","ZOOM","↓","↓","ZOOM"],"section":"DATE"},
    {"id":"DATE_TIME_LINK_TO_OFFSET_DATE_TIME","label":"DLNK","path":["HOME","DATE","DT","DLNK"],"route":["MATH","↓","↓","GRAPH","ZOOM","↓","↓","TRACE"],"section":"DATE"},
    {"id":"OFFSET_CREATE","label":"TZ{}","path":["HOME","DATE","TZ","TZ{}"],"route":["MATH","↓","↓","GRAPH","TRACE","Y="],"section":"DATE"},
    {"id":"OFFSET_TO_HOURS","label":"→HR","path":["HOME","DATE","TZ","→HR"],"route":["MATH","↓","↓","GRAPH","TRACE","WINDOW"],"section":"DATE"},
    {"id":"HOURS_TO_OFFSET","label":"HR→","path":["HOME","DATE","TZ","HR→"],"route":["MATH","↓","↓","GRAPH","TRACE","ZOOM"],"section":"DATE"},
    {"id":"OFFSET_EXTRACT_HOUR","label":".HR","path":["HOME","DATE","TZ",".HR"],"route":["MATH","↓","↓","GRAPH","TRACE","TRACE"],"section":"DATE"},
    {"id":"OFFSET_EXTRACT_MINUTE","label":".MIN","path":["HOME","DATE","TZ",".MIN"],"route":["MATH","↓","↓","GRAPH","TRACE","GRAPH"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_CREATE","label":"DZ{}","path":["HOME","DATE","DZ","DZ{}"],"route":["MATH","↓","↓","GRAPH","GRAPH","Y="],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_TO_EPOCH_DAYS","label":"→ED","path":["HOME","DATE","DZ","→ED"],"route":["MATH","↓","↓","GRAPH","GRAPH","WINDOW"],"section":"DATE"},
    {"id":"EPOCH_DAYS_TO_OFFSET_DATE_TIME","label":"ED→","path":["HOME","DATE","DZ","ED→"],"route":["MATH","↓","↓","GRAPH","GRAPH","ZOOM"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_TO_EPOCH_SECONDS","label":"→ES","path":["HOME","DATE","DZ","→ES"],"route":["MATH","↓","↓","GRAPH","GRAPH","TRACE"],"section":"DATE"},
    {"id":"EPOCH_SECONDS_TO_OFFSET_DATE_TIME_UTC","label":"ES→","path":["HOME","DATE","DZ","ES→"],"route":["MATH","↓","↓","GRAPH","GRAPH","GRAPH"],"section":"DATE"},
    {"id":"EPOCH_SECONDS_TO_OFFSET_DATE_TIME_APP","label":"ES→@","path":["HOME","DATE","DZ","ES→@"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","Y="],"section":"DATE"},
    {"id":"IS_OFFSET_DATE_TIME_LEAP","label":"LEAP","path":["HOME","DATE","DZ","LEAP"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","WINDOW"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_TO_DAY_OF_WEEK","label":"DOW","path":["HOME","DATE","DZ","DOW"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","ZOOM"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_CONVERT_TO_TIME_ZONE","label":"CVTZ","path":["HOME","DATE","DZ","CVTZ"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","TRACE"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_EXTRACT_DATE","label":".D{}","path":["HOME","DATE","DZ",".D{}"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","↓","Y="],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_EXTRACT_TIME","label":".T{}","path":["HOME","DATE","DZ",".T{}"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","↓","WINDOW"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_EXTRACT_DATE_TIME","label":".DT{","path":["HOME","DATE","DZ",".DT{"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","↓","ZOOM"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_EXTRACT_OFFSET","label":".TZ{","path":["HOME","DATE","DZ",".TZ{"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","↓","TRACE"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_SHRINK_TO_DATE_TIME","label":"DSHK","path":["HOME","DATE","DZ","DSHK"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","↓","↓","Y="],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_EXTEND_TO_NOTHING","label":"DEXD","path":["HOME","DATE","DZ","DEXD"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","↓","↓","WINDOW"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_CUT_TO_DATE_TIME","label":"DCUT","path":["HOME","DATE","DZ","DCUT"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","↓","↓","ZOOM"],"section":"DATE"},
    {"id":"OFFSET_DATE_TIME_LINK_TO_NOTHING","label":"DLNK","path":["HOME","DATE","DZ","DLNK"],"route":["MATH","↓","↓","GRAPH","GRAPH","↓","↓","↓","TRACE"],"section":"DATE"},
    {"id":"DURATION_CREATE","label":"DR{}","path":["HOME","DATE","DR","DR{}"],"route":["MATH","↓","↓","GRAPH","↓","Y=","Y="],"section":"DATE"},
    {"id":"DAYS_TO_DURATION","label":"DAY→","path":["HOME","DATE","DR","DAY→"],"route":["MATH","↓","↓","GRAPH","↓","Y=","WINDOW"],"section":"DATE"},
    {"id":"HOURS_TO_DURATION","label":"HR→","path":["HOME","DATE","DR","HR→"],"route":["MATH","↓","↓","GRAPH","↓","Y=","ZOOM"],"section":"DATE"},
    {"id":"MINUTES_TO_DURATION","label":"MIN→","path":["HOME","DATE","DR","MIN→"],"route":["MATH","↓","↓","GRAPH","↓","Y=","TRACE"],"section":"DATE"},
    {"id":"SECONDS_TO_DURATION","label":"SEC→","path":["HOME","DATE","DR","SEC→"],"route":["MATH","↓","↓","GRAPH","↓","Y=","GRAPH"],"section":"DATE"},
    {"id":"DURATION_TO_SECONDS","label":"→S","path":["HOME","DATE","DR","→S"],"route":["MATH","↓","↓","GRAPH","↓","Y=","↓","Y="],"section":"DATE"},
    {"id":"DURATION_EXTRACT_DAY","label":".DAY","path":["HOME","DATE","DR",".DAY"],"route":["MATH","↓","↓","GRAPH","↓","Y=","↓","WINDOW"],"section":"DATE"},
    {"id":"DURATION_EXTRACT_HOUR","label":".HR","path":["HOME","DATE","DR",".HR"],"route":["MATH","↓","↓","GRAPH","↓","Y=","↓","ZOOM"],"section":"DATE"},
    {"id":"DURATION_EXTRACT_MINUTE","label":".MIN","path":["HOME","DATE","DR",".MIN"],"route":["MATH","↓","↓","GRAPH","↓","Y=","↓","TRACE"],"section":"DATE"},
    {"id":"DURATION_EXTRACT_SECOND","label":".SEC","path":["HOME","DATE","DR",".SEC"],"route":["MATH","↓","↓","GRAPH","↓","Y=","↓","GRAPH"],"section":"DATE"},
    {"id":"DAY_OF_WEEK_CREATE","label":"DW{}","path":["HOME","DATE","DW","DW{}"],"route":["MATH","↓","↓","GRAPH","↓","WINDOW","Y="],"section":"DATE"},
    {"id":"DAY_OF_WEEK_TO_ISO_DOW_NUMBER","label":"→ISO","path":["HOME","DATE","DW","→ISO"],"route":["MATH","↓","↓","GRAPH","↓","WINDOW","WINDOW"],"section":"DATE"},
    {"id":"ISO_DOW_NUMBER_TO_DAY_OF_WEEK","label":"ISO→","path":["HOME","DATE","DW","ISO→"],"route":["MATH","↓","↓","GRAPH","↓","WINDOW","ZOOM"],"section":"DATE"},
    {"id":"DAY_OF_WEEK_TO_UNIX_DOW_NUMBER","label":"→UNX","path":["HOME","DATE","DW","→UNX"],"route":["MATH","↓","↓","GRAPH","↓","WINDOW","TRACE"],"section":"DATE"},
    {"id":"UNIX_DOW_NUMBER_TO_DAY_OF_WEEK","label":"UNX→","path":["HOME","DATE","DW","UNX→"],"route":["MATH","↓","↓","GRAPH","↓","WINDOW","GRAPH"],"section":"DATE"},
    {"id":"EPOCH_UNIX","label":"UNIX","path":["HOME","DATE","EPCH","UNIX"],"route":["MATH","↓","↓","GRAPH","↓","ZOOM","Y="],"section":"DATE"},
    {"id":"EPOCH_NTP","label":"NTP","path":["HOME","DATE","EPCH","NTP"],"route":["MATH","↓","↓","GRAPH","↓","ZOOM","WINDOW"],"section":"DATE"},
    {"id":"EPOCH_GPS","label":"GPS","path":["HOME","DATE","EPCH","GPS"],"route":["MATH","↓","↓","GRAPH","↓","ZOOM","ZOOM"],"section":"DATE"},
    {"id":"EPOCH_TIOS","label":"TIOS","path":["HOME","DATE","EPCH","TIOS"],"route":["MATH","↓","↓","GRAPH","↓","ZOOM","TRACE"],"section":"DATE"},
    {"id":"EPOCH_Y2K","label":"Y2K","path":["HOME","DATE","EPCH","Y2K"],"route":["MATH","↓","↓","GRAPH","↓","ZOOM","GRAPH"],"section":"DATE"},
    {"id":"EPOCH_CUSTOM","label":"CEPC","path":["HOME","DATE","EPCH","CEPC"],"route":["MATH","↓","↓","GRAPH","↓","ZOOM","↓","Y="],"section":"DATE"},
    {"id":"EPOCH_SET_CUSTOM","label":"EPC","path":["HOME","DATE","EPCH","EPC"],"route":["MATH","↓","↓","GRAPH","↓","ZOOM","↓","TRACE"],"section":"DATE"},
    {"id":"EPOCH_GET_CUSTOM","label":"EPC?","path":["HOME","DATE","EPCH","EPC?"],"route":["MATH","↓","↓","GRAPH","↓","ZOOM","↓","GRAPH"],"section":"DATE"},
    {"id":"GET_NOW","label":"NOW","path":["HOME","DATE","CLK","NOW"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","Y="],"section":"DATE"},
    {"id":"GET_NOW_DATE","label":"NOWD","path":["HOME","DATE","CLK","NOWD"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","WINDOW"],"section":"DATE"},
    {"id":"GET_NOW_TIME","label":"NOWT","path":["HOME","DATE","CLK","NOWT"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","ZOOM"],"section":"DATE"},
    {"id":"GET_NOW_OFFSET_DATE_TIME","label":"NWDZ","path":["HOME","DATE","CLK","NWDZ"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","TRACE"],"section":"DATE"},
    {"id":"GET_NOW_OFFSET_DATE_TIME_UTC","label":"NWUT","path":["HOME","DATE","CLK","NWUT"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","GRAPH"],"section":"DATE"},
    {"id":"SET_APP_TIME_ZONE","label":"ATZ","path":["HOME","DATE","CLK","ATZ"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","↓","Y="],"section":"DATE"},
    {"id":"GET_APP_TIME_ZONE","label":"ATZ?","path":["HOME","DATE","CLK","ATZ?"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","↓","WINDOW"],"section":"DATE"},
    {"id":"SET_CLOCK_TIME_ZONE","label":"CTZ","path":["HOME","DATE","CLK","CTZ"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","↓","ZOOM"],"section":"DATE"},
    {"id":"GET_CLOCK_TIME_ZONE","label":"CTZ?","path":["HOME","DATE","CLK","CTZ?"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","↓","TRACE"],"section":"DATE"},
    {"id":"SET_CLOCK","label":"SETC","path":["HOME","DATE","CLK","SETC"],"route":["MATH","↓","↓","GRAPH","↓","TRACE","↓","GRAPH"],"section":"DATE"}
  ],
  "units": [
    {"id":"UNIT_FERMI","base":"UNIT_METER","dimension":"LENGTH","value":"0.000000000000001"},
    {"id":"UNIT_ANGSTROM","base":"UNIT_METER","dimension":"LENGTH","value":"0.0000000001"},
    {"id":"UNIT_NANO_METER","base":"UNIT_METER","dimension":"LENGTH","value":"0.000000001"},
    {"id":"UNIT_MICRO_METER","base":"UNIT_METER","dimension":"LENGTH","value":"0.000001"},
    {"id":"UNIT_MILLI_METER","base":"UNIT_METER","dimension":"LENGTH","value":"0.001"},
    {"id":"UNIT_CENTI_METER","base":"UNIT_METER","dimension":"LENGTH","value":"0.01"},
    {"id":"UNIT_METER","base":"UNIT_METER","dimension":"LENGTH","value":"1.0"},
    {"id":"UNIT_KILO_METER","base":"UNIT_METER","dimension":"LENGTH","value":"1000.0"},
    {"id":"UNIT_MIL","base":"UNIT_METER","dimension":"LENGTH","value":"0.0000254"},
    {"id":"UNIT_INCH","base":"UNIT_METER","dimension":"LENGTH","value":"0.0254"},
    {"id":"UNIT_FOOT","base":"UNIT_METER","dimension":"LENGTH","value":"0.3048"},
    {"id":"UNIT_YARD","base":"UNIT_METER","dimension":"LENGTH","value":"0.9144"},
    {"id":"UNIT_MILE","base":"UNIT_METER","dimension":"LENGTH","value":"1609.344"},
    {"id":"UNIT_TWIP","base":"UNIT_METER","dimension":"LENGTH","value":"0.00001763888888888888888888888888888888888888888888888888889"},
    {"id":"UNIT_POINT","base":"UNIT_METER","dimension":"LENGTH","value":"0.0003527777777777777777777777777777777777777777777777777778"},
    {"id":"UNIT_PICA","base":"UNIT_METER","dimension":"LENGTH","value":"0.004233333333333333333333333333333333333333333333333333333"},
    {"id":"UNIT_SURVEY_FOOT","base":"UNIT_METER","dimension":"LENGTH","value":"0.3048006096012192024384048768097536195072390144780289561"},
    {"id":"UNIT_ROD","base":"UNIT_METER","dimension":"LENGTH","value":"5.029210058420116840233680467360934721869443738887477775"},
    {"id":"UNIT_CHAIN","base":"UNIT_METER","dimension":"LENGTH","value":"20.1168402336804673609347218694437388874777749555499111"},
    {"id":"UNIT_FURLONG","base":"UNIT_METER","dimension":"LENGTH","value":"201.168402336804673609347218694437388874777749555499111"},
    {"id":"UNIT_SURVEY_MILE","base":"UNIT_METER","dimension":"LENGTH","value":"1609.347218694437388874777749555499110998221996443992888"},
    {"id":"UNIT_LEAGUE","base":"UNIT_METER","dimension":"LENGTH","value":"4828.041656083312166624333248666497332994665989331978664"},
    {"id":"UNIT_FATHOM","base":"UNIT_METER","dimension":"LENGTH","value":"1.8288"},
    {"id":"UNIT_CABLE","base":"UNIT_METER","dimension":"LENGTH","value":"219.456"},
    {"id":"UNIT_NAUTICAL_MILE","base":"UNIT_METER","dimension":"LENGTH","value":"1852.0"},
    {"id":"UNIT_LIGHT_SECOND","base":"UNIT_METER","dimension":"LENGTH","value":"299792458.0"},
    {"id":"UNIT_ASTRONOMICAL_UNIT","base":"UNIT_METER","dimension":"LENGTH","value":"149597870700.0"},
    {"id":"UNIT_LIGHT_YEAR","base":"UNIT_METER","dimension":"LENGTH","value":"9460730472580800.0"},
    {"id":"UNIT_PARSEC","base":"UNIT_METER","dimension":"LENGTH","value":"30856775814913672.78913937957796471610731921160409179801"},
    {"id":"UNIT_SQ_MILLI_METER","base":"UNIT_SQ_METER","dimension":"AREA","value":"0.000001"},
    {"id":"UNIT_SQ_CENTI_METER","base":"UNIT_SQ_METER","dimension":"AREA","value":"0.0001"},
    {"id":"UNIT_SQ_METER","base":"UNIT_SQ_METER","dimension":"AREA","value":"1.0"},
    {"id":"UNIT_SQ_KILO_METER","base":"UNIT_SQ_METER","dimension":"AREA","value":"1000000.0"},
    {"id":"UNIT_SQ_INCH","base":"UNIT_SQ_METER","dimension":"AREA","value":"0.00064516"},
    {"id":"UNIT_SQ_FOOT","base":"UNIT_SQ_METER","dimension":"AREA","value":"0.09290304"},
    {"id":"UNIT_SQ_YARD","base":"UNIT_SQ_METER","dimension":"AREA","value":"0.83612736"},
    {"id":"UNIT_SQ_MILE","base":"UNIT_SQ_METER","dimension":"AREA","value":"2589988.110336"},
    {"id":"UNIT_SQ_NAUTICAL_MILE","base":"UNIT_SQ_METER","dimension":"AREA","value":"3429904.0"},
    {"id":"UNIT_SQ_ROD","base":"UNIT_SQ_METER","dimension":"AREA","value":"25.29295381171407504105330791306743903810388265937822198"},
    {"id":"UNIT_SQ_CHAIN","base":"UNIT_SQ_METER","dimension":"AREA","value":"404.6872609874252006568529266090790246096621225500515517"},
    {"id":"UNIT_SQ_FURLONG","base":"UNIT_SQ_METER","dimension":"AREA","value":"40468.72609874252006568529266090790246096621225500515517"},
    {"id":"UNIT_ACRE","base":"UNIT_SQ_METER","dimension":"AREA","value":"4046.8564224"},
    {"id":"UNIT_HECTARE","base":"UNIT_SQ_METER","dimension":"AREA","value":"10000.0"},
    {"id":"UNIT_USFOOTBALL","base":"UNIT_SQ_METER","dimension":"AREA","value":"5351.215104"},
    {"id":"UNIT_CAFOOTBALL","base":"UNIT_SQ_METER","dimension":"AREA","value":"8152.24176"},
    {"id":"UNIT_CU_MILLI_METER","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000000001"},
    {"id":"UNIT_CU_CENTI_METER","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000001"},
    {"id":"UNIT_CU_METER","base":"UNIT_CU_METER","dimension":"VOLUME","value":"1.0"},
    {"id":"UNIT_CU_KILO_METER","base":"UNIT_CU_METER","dimension":"VOLUME","value":"1000000000.0"},
    {"id":"UNIT_CU_INCH","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000016387064"},
    {"id":"UNIT_CU_FOOT","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.028316846592"},
    {"id":"UNIT_CU_YARD","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.764554857984"},
    {"id":"UNIT_CU_MILE","base":"UNIT_CU_METER","dimension":"VOLUME","value":"4168181825.440579584"},
    {"id":"UNIT_CU_NAUTICAL_MILE","base":"UNIT_CU_METER","dimension":"VOLUME","value":"6352182208.0"},
    {"id":"UNIT_MICRO_LITER","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000000001"},
    {"id":"UNIT_MILLI_LITER","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000001"},
    {"id":"UNIT_LITER","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.001"},
    {"id":"UNIT_METRIC_TEASPOON","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000005"},
    {"id":"UNIT_METRIC_TABLESPOON","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000015"},
    {"id":"UNIT_TEASPOON","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.00000492892159375"},
    {"id":"UNIT_TABLESPOON","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.00001478676478125"},
    {"id":"UNIT_FLUID_OUNCE","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.0000295735295625"},
    {"id":"UNIT_GILL","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.00011829411825"},
    {"id":"UNIT_CUP","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.0002365882365"},
    {"id":"UNIT_PINT","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000473176473"},
    {"id":"UNIT_QUART","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000946352946"},
    {"id":"UNIT_GALLON","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.003785411784"},
    {"id":"UNIT_IMP_TEASPOON","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000005919388020833333333333333333333333333333333333333333333"},
    {"id":"UNIT_IMP_TABLESPOON","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.0000177581640625"},
    {"id":"UNIT_IMP_FLUID_OUNCE","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.0000284130625"},
    {"id":"UNIT_IMP_GILL","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.0001420653125"},
    {"id":"UNIT_IMP_CUP","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.000284130625"},
    {"id":"UNIT_IMP_PINT","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.00056826125"},
    {"id":"UNIT_IMP_QUART","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.0011365225"},
    {"id":"UNIT_IMP_GALLON","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.00454609"},
    {"id":"UNIT_DRY_PINT","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.0005506104713575"},
    {"id":"UNIT_DRY_QUART","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.001101220942715"},
    {"id":"UNIT_DRY_GALLON","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.00440488377086"},
    {"id":"UNIT_PECK","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.00880976754172"},
    {"id":"UNIT_BUSHEL","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.03523907016688"},
    {"id":"UNIT_DRY_BARREL","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.115627123584"},
    {"id":"UNIT_BOARD_FOOT","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.002359737216"},
    {"id":"UNIT_OIL_BARREL","base":"UNIT_CU_METER","dimension":"VOLUME","value":"0.158987294928"},
    {"id":"UNIT_OLYMPIC_POOL","base":"UNIT_CU_METER","dimension":"VOLUME","value":"2500.0"},
    {"id":"UNIT_ACRE_FOOT","base":"UNIT_CU_METER","dimension":"VOLUME","value":"1233.48183754752"},
    {"id":"UNIT_CELSIUS","base":"UNIT_KELVIN","dimension":"TEMP","value":"274.15"},
    {"id":"UNIT_FAHRENHEIT","base":"UNIT_KELVIN","dimension":"TEMP","value":"255.9277777777777777777777777777777777777777777777777778"},
    {"id":"UNIT_RANKINE","base":"UNIT_KELVIN","dimension":"TEMP","value":"0.5555555555555555555555555555555555555555555555555555556"},
    {"id":"UNIT_KELVIN","base":"UNIT_KELVIN","dimension":"TEMP","value":"1.0"},
    {"id":"UNIT_MICRO_GRAM","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.000000001"},
    {"id":"UNIT_MILLI_GRAM","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.000001"},
    {"id":"UNIT_GRAM","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.001"},
    {"id":"UNIT_KILO_GRAM","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"1.0"},
    {"id":"UNIT_METRIC_TON","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"1000.0"},
    {"id":"UNIT_ATOMIC_MASS_UNIT","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"1.66053906892e-27"},
    {"id":"UNIT_GRAIN","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.00006479891"},
    {"id":"UNIT_DRAM","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.0017718451953125"},
    {"id":"UNIT_OUNCE","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.028349523125"},
    {"id":"UNIT_POUND","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.45359237"},
    {"id":"UNIT_SLUG","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"14.59390293720636482939632545931758530183727034120734908"},
    {"id":"UNIT_SHORT_HUNDRED_WEIGHT","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"45.359237"},
    {"id":"UNIT_SHORT_TON","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"907.18474"},
    {"id":"UNIT_IMP_POUND","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.45359237"},
    {"id":"UNIT_STONE","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"6.35029318"},
    {"id":"UNIT_QUARTER","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"12.70058636"},
    {"id":"UNIT_LONG_HUNDRED_WEIGHT","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"50.80234544"},
    {"id":"UNIT_LONG_TON","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"1016.0469088"},
    {"id":"UNIT_TROY_GRAIN","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.00006479891"},
    {"id":"UNIT_TROY_PENNY_WEIGHT","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.00155517384"},
    {"id":"UNIT_TROY_OUNCE","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.0311034768"},
    {"id":"UNIT_TROY_POUND","base":"UNIT_KILO_GRAM","dimension":"MASS","value":"0.3732417216"},
    {"id":"UNIT_DYNE","base":"UNIT_NEWTON","dimension":"FORCE","value":"0.00001"},
    {"id":"UNIT_NEWTON","base":"UNIT_NEWTON","dimension":"FORCE","value":"1.0"},
    {"id":"UNIT_KILOGRAM_FORCE","base":"UNIT_NEWTON","dimension":"FORCE","value":"9.80665"},
    {"id":"UNIT_METRIC_TON_FORCE","base":"UNIT_NEWTON","dimension":"FORCE","value":"9806.65"},
    {"id":"UNIT_POUNDAL","base":"UNIT_NEWTON","dimension":"FORCE","value":"0.138254954376"},
    {"id":"UNIT_POUND_FORCE","base":"UNIT_NEWTON","dimension":"FORCE","value":"4.4482216152605"},
    {"id":"UNIT_SHORT_TON_FORCE","base":"UNIT_NEWTON","dimension":"FORCE","value":"8896.443230521"},
    {"id":"UNIT_LONG_TON_FORCE","base":"UNIT_NEWTON","dimension":"FORCE","value":"9964.01641818352"},
    {"id":"UNIT_PASCAL","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"1.0"},
    {"id":"UNIT_HECTO_PASCAL","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"100.0"},
    {"id":"UNIT_KILO_PASCAL","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"1000.0"},
    {"id":"UNIT_TORR","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"133.3223684210526315789473684210526315789473684210526316"},
    {"id":"UNIT_ATMOSPHERE","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"101325.0"},
    {"id":"UNIT_MILLI_BAR","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"100.0"},
    {"id":"UNIT_DECI_BAR","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"10000.0"},
    {"id":"UNIT_BAR","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"100000.0"},
    {"id":"UNIT_POUND_SQUARE_INCH","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"6894.75729316836133672267344534689069378138756277512555"},
    {"id":"UNIT_MILLI_METER_MERCURY","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"133.322387415"},
    {"id":"UNIT_INCH_MERCURY","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"3386.388640341"},
    {"id":"UNIT_MILLI_METER_WATER","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"9.80665"},
    {"id":"UNIT_INCH_WATER","base":"UNIT_PASCAL","dimension":"PRESSURE","value":"249.08891"},
    {"id":"UNIT_ELECTRON_VOLT","base":"UNIT_JOULE","dimension":"ENERGY","value":"1.602176634e-19"},
    {"id":"UNIT_ERG","base":"UNIT_JOULE","dimension":"ENERGY","value":"0.0000001"},
    {"id":"UNIT_JOULE","base":"UNIT_JOULE","dimension":"ENERGY","value":"1.0"},
    {"id":"UNIT_KILO_JOULE","base":"UNIT_JOULE","dimension":"ENERGY","value":"1000.0"},
    {"id":"UNIT_FOOT_POUND_ENERGY","base":"UNIT_JOULE","dimension":"ENERGY","value":"1.3558179483314004"},
    {"id":"UNIT_WATT_HOUR","base":"UNIT_JOULE","dimension":"ENERGY","value":"3600.0"},
    {"id":"UNIT_KILO_WATT_HOUR","base":"UNIT_JOULE","dimension":"ENERGY","value":"3600000.0"},
    {"id":"UNIT_CALORIE","base":"UNIT_JOULE","dimension":"ENERGY","value":"4.184"},
    {"id":"UNIT_KILO_CALORIE","base":"UNIT_JOULE","dimension":"ENERGY","value":"4184.0"},
    {"id":"UNIT_BRITISH_THERMAL_UNIT","base":"UNIT_JOULE","dimension":"ENERGY","value":"1055.05585262"},
    {"id":"UNIT_TON_TNT","base":"UNIT_JOULE","dimension":"ENERGY","value":"4184000000.0"},
    {"id":"UNIT_LITER_ATMOSPHERE","base":"UNIT_JOULE","dimension":"ENERGY","value":"101.325"},
    {"id":"UNIT_WATT","base":"UNIT_WATT","dimension":"POWER","value":"1.0"},
    {"id":"UNIT_KILO_WATT","base":"UNIT_WATT","dimension":"POWER","value":"1000.0"},
    {"id":"UNIT_FOOT_POUND_ENERGY_PER_SECOND","base":"UNIT_WATT","dimension":"POWER","value":"1.3558179483314004"},
    {"id":"UNIT_BTU_PER_HOUR","base":"UNIT_WATT","dimension":"POWER","value":"0.2930710701722222222222222222222222222222222222222222222"},
    {"id":"UNIT_BTU_PER_MINUTE","base":"UNIT_WATT","dimension":"POWER","value":"17.58426421033333333333333333333333333333333333333333333"},
    {"id":"UNIT_CALORIE_PER_SECOND","base":"UNIT_WATT","dimension":"POWER","value":"4.184"},
    {"id":"UNIT_HORSEPOWER","base":"UNIT_WATT","dimension":"POWER","value":"745.69987158227022"},
    {"id":"UNIT_NANO_SECOND","base":"UNIT_SECOND","dimension":"TIME","value":"0.000000001"},
    {"id":"UNIT_MICRO_SECOND","base":"UNIT_SECOND","dimension":"TIME","value":"0.000001"},
    {"id":"UNIT_MILLI_SECOND","base":"UNIT_SECOND","dimension":"TIME","value":"0.001"},
    {"id":"UNIT_SECOND","base":"UNIT_SECOND","dimension":"TIME","value":"1.0"},
    {"id":"UNIT_MINUTE","base":"UNIT_SECOND","dimension":"TIME","value":"60.0"},
    {"id":"UNIT_HOUR","base":"UNIT_SECOND","dimension":"TIME","value":"3600.0"},
    {"id":"UNIT_DAY","base":"UNIT_SECOND","dimension":"TIME","value":"86400.0"},
    {"id":"UNIT_WEEK","base":"UNIT_SECOND","dimension":"TIME","value":"604800.0"},
    {"id":"UNIT_JULIAN_MONTH","base":"UNIT_SECOND","dimension":"TIME","value":"2629800.0"},
    {"id":"UNIT_JULIAN_YEAR","base":"UNIT_SECOND","dimension":"TIME","value":"31557600.0"},
    {"id":"UNIT_METER_PER_SECOND","base":"UNIT_METER_PER_SECOND","dimension":"SPEED","value":"1.0"},
    {"id":"UNIT_FOOT_PER_SECOND","base":"UNIT_METER_PER_SECOND","dimension":"SPEED","value":"0.3048"},
    {"id":"UNIT_KILO_METER_PER_HOUR","base":"UNIT_METER_PER_SECOND","dimension":"SPEED","value":"0.2777777777777777777777777777777777777777777777777777778"},
    {"id":"UNIT_MILE_PER_HOUR","base":"UNIT_METER_PER_SECOND","dimension":"SPEED","value":"0.44704"},
    {"id":"UNIT_KNOT","base":"UNIT_METER_PER_SECOND","dimension":"SPEED","value":"0.5144444444444444444444444444444444444444444444444444444"},
    {"id":"UNIT_LIGHT_SPEED","base":"UNIT_METER_PER_SECOND","dimension":"SPEED","value":"299792458.0"},
    {"id":"UNIT_LITERS_PER_HUNDRED_KILO_METERS","base":"UNIT_LITERS_PER_HUNDRED_KILO_METERS","dimension":"FUEL","value":"1.0"},
    {"id":"UNIT_MILES_PER_GALLON","base":"UNIT_LITERS_PER_HUNDRED_KILO_METERS","dimension":"FUEL","value":"235.2145833333333333333333333333333333333333333333333333"}
  ],
  "menuDigest": "e89ce127dc7925ccb5643a7bda0eb604f96277d1f9e3744d44260691be3b6a58",
  "references": {"sinh230":"6.36572807683162355302753470060147645626906263536057793e+99","sinh1":"1.17520119364380145688238185059560081515571798133409587","cosh1":"1.543080634815243778477905620757061682601529112365863705","tanh1":"0.7615941559557648881194582826047935904127685972579365516","asinh1":"0.8813735870195430252326093249797923090281603282616354108","acosh2":"1.31695789692481670862504634730796844402698197146751648","atanh05":"0.5493061443340548456976226184612628523237452789113747259","atan2":"0.9272952180016122324285124629224288040570741085722405276","sqrt2":"1.414213562373095048801688724209698078569671875376948073","sqrt23":"0.8164965809277260327324280249019637973219824935522233761","ln2":"0.6931471805599453094172321214581765680755001343602552541","log10two":"0.3010299956639811952137388947244930267681898814621085413","e":"2.718281828459045235360287471352662497757247093699959575","pi":"3.141592653589793238462643383279502884197169399375105821","expm1tiny":"0.000000000001000000000000500000000000166666666666708333333333341667","log1ptiny":"0.0000000000009999999999995000000000003333333333330833333333335333333","mortgage":"-1264.13604698592746409166335247731953187583760823415496","growth":"1628.89462677744140625","lnsum":"1.791759469228055000812477358380702272722990692183004706","lnx2":"1.687401974730783402510881650176030885349016298223767572","lnysum":"3.871201010907890929064173722755231976949491095263770468","lny2":"5.612667065159408498704639715482363515286384302901221262","lnxlny":"2.929354999067793813607106893339199742721870873170676036","xlny":"8.841014310483891549523592682432989658546472748350290388","ylnx":"9.364262454248439386040399907367860500186943884377517727","energychange":"16.22222222222222222222222222222222222222222222222222222","fact69":"1.711224524281413113724683388812728390922705448935203694e+98","weightedX":"2.333333333333333333333333333333333333333333333333333333"}
};
const cases = [];
const byId = Object.fromEntries(source.commands.map(c => [c.id, c]));
const ref = source.references;
function menu(id) { return byId[id].path.join(' → '); }
function select(id) { return `Select ${menu(id)}.`; }
const exact = value => ({ kind: 'number', value: String(value), relative: '0', absolute: '0' });
const approx = (value, relative = '5E-11', absolute = '0') => ({ kind: 'number', value: String(value), relative, absolute });
const text = value => ({ kind: 'text', value });
const error = message => ({ kind: 'error', values: [message] });
function add(id, section, title, steps, expected, covers = [], options = {}) {
  cases.push({ id, section, title, steps, expected: expected || null, covers, kind: expected ? 'result' : 'observation', focus: 'normal', checks: [], reference: expected?.kind === 'number' ? 'Decimal arithmetic or a 70-digit mathematical reference; see the reference notes.' : 'Current user guide and menu definitions.', ...options });
}
function command(id, input, value, options = {}) {
  const { suffix = [], setup = [], ...rest } = options;
  add(`command-${id.toLowerCase()}`, byId[id].section, `${byId[id].label}: ${input}`, [...setup, `Enter ${input}.`, select(id), ...suffix], value, [id], rest);
}

for (const g of source.groups) {
  add(`menu-${g.id.toLowerCase()}`, g.section, `${g.path.join(' → ')}: menu rows`, [
    `Open this menu with physical keys: ${g.route.join(' → ')}.`,
    ...g.rows.map((r, i) => `Row ${i + 1}: ${r.join(' | ')}.${i < g.rows.length - 1 ? ' Press ↓ once for the next row.' : ''}`),
    `${g.id === 'HOME' ? 'ON stays in HOME.' : 'Press ON to return one level.'} Do not execute the individual commands in this check.`
  ], null, [], { kind: 'navigation', checks: ['Every row has these labels in these five positions (a filled square may mark the selected mode).', `Rows change cleanly and ON ${g.id === 'HOME' ? 'stays in HOME' : 'returns to the parent'} without changing the stack.`] });
}

// Each conversion is checked numerically; checking a menu alone never counts here.
for (const u of source.units) {
  add(`unit-${u.id.toLowerCase()}`, 'UNIT', `1 ${byId[u.id].label} → ${byId[u.base].label}`, [
    'Enter 1.', select(u.id), `Check that X is labelled ${byId[u.id].label}.`, select(u.base),
    'Check the destination unit before opening SHOW; enter only the numeric magnitude as the answer.'
  ], approx(u.value), [u.id, u.base], { checks: [`X has the destination unit ${byId[u.base].label}.`], reference: `Exact ${u.dimension.toLowerCase()} definitions from tools/genunits.py, evaluated separately to 55 decimal digits. These checks share the definitions with the program and do not independently audit those definitions.` });
}

for (const [id, input, value] of [
  ['CUBE','3',27], ['CUBE_ROOT','8 (−)',-2], ['XROOT_Y','8 (−) ENTER 3',-2],
  ['TWO_POW','10',1024], ['LOG2','1024',10], ['LOG_BASE','81 ENTER 3',4],
  ['PERCENT','200 ENTER 15',30], ['PERCENT_CHANGE','80 ENTER 100',25],
  ['GCD','84 ENTER 30',6], ['LCM','6 ENTER 8',24],
  ['INT_PART','2.75 (−)',-2], ['FRAC_PART','2.75 (−)','-.75'], ['FLOOR','2.75 (−)',-3],
  ['CEIL','2.75 (−)',-2], ['NEAR','2.5 (−)',-3], ['ABS','12.5 (−)','12.5'], ['SIGN','12.5 (−)',-1],
  ['MOD','17 ENTER 5',2], ['MIN','17 ENTER 5',5], ['MAX','17 ENTER 5',17],
  ['COMB','10 ENTER 3',120], ['PERM','10 ENTER 3',720], ['FACTORIAL','0',1],
  ['RAD_TO_DEG','0',0], ['DEG_TO_RAD','180',null],
  ['HMS_TO_HR','1.3045','1.5125'], ['HR_TO_HMS','1.5125','1.3045'],
  ['HMS_PLUS','1.45 ENTER .30','2.15'], ['HMS_MINUS','2.15 ENTER .30','1.45']
]) command(id, input, value === null ? approx(ref.pi) : ['CUBE_ROOT','XROOT_Y','LOG2','LOG_BASE'].includes(id) ? approx(value) : exact(value), { checks: ['PERCENT','PERCENT_CHANGE'].includes(id) ? ['The original value remains in Y.'] : [] });
command('ATAN2','4 ENTER 3',approx(ref.atan2));
command('EXP_MINUS_ONE','1 EE (−) 12',approx(ref.expm1tiny));
command('LN_ONE_PLUS','1 EE (−) 12',approx(ref.log1ptiny));
command('PRIME','1981',exact(7),{ checks:['Y still contains 1981.'] });
add('prime-prime','NUM','PRIM on a prime', ['Enter 1999.',select('PRIME')],exact(1),['PRIME'],{checks:['Y still contains 1999.']});
add('lcm-zero','NUM','LCM with zero', ['Enter 0 ENTER 8.',select('LCM')],exact(0),['LCM']);
command('ROUND_TO_N','2.675',exact('2.68'),{suffix:['At the RNDN prompt press 2.']});
command('ROUND_TO_GUARD','12345',exact(12300),{suffix:['At the RNDG prompt press 3.']});
command('ROUND_TO_FIX','2.675',exact('2.68'),{setup:[select('FIX'),'At the FIX prompt press 2.'],suffix:[select('FIX'),'At the FIX prompt press . then 2 to restore all display digits.']});
for (const [id,input,r] of [['SINH','1','sinh1'],['COSH','1','cosh1'],['TANH','1','tanh1'],['ASINH','1','asinh1'],['ACOSH','2','acosh2'],['ATANH','.5','atanh05']]) command(id,input,approx(ref[r]));
command('RECT_TO_POLAR','4 ENTER 3',exact(5),{ checks:['Y is approximately 0.92729521800161 radians.'] });
command('POLAR_TO_RECT','0 ENTER 5',exact(5),{checks:['Y is 0.']});
command('RANDOM','0',{kind:'range',min:'0',max:'1'}, {checks:['Repeating RAND several times gives values within [0, 1), without an error.']});
add('random-seed','PROB','Repeatable random seed', ['Enter 12345.',select('RANDOM_SEED'),select('RANDOM'),'Write the first result in the notes. Enter 12345 again.',select('RANDOM_SEED'),select('RANDOM')],null,['RANDOM_SEED'],{checks:['Reseeding gives exactly the same first random value.']});
const unavailable = ['COMPLEX_REAL','COMPLEX_IMAG','COMPLEX_CONJ','COMPLEX_ABS','COMPLEX_ANGLE','NUM_RESULT_MODE_REAL','NUM_RESULT_MODE_COMPLEX','COMPLEX_MODE_RECT','COMPLEX_MODE_RAD','COMPLEX_MODE_DEG'];
for (const id of unavailable) {
  command(id,'2',null,{focus:'availability',checks:['The message is “Not available yet”; X stays 2 and the program remains responsive.']});
}

// Use decimal display so bitwise results can be entered without translating a base.
const baseSetup = ['Enter 8.',select('SET_WORD_SIZE'),select('CLEAR_CARRY_FLAG')];
for (const [id,input,value] of [
  ['BITWISE_AND','240 ENTER 60',48],['BITWISE_OR','240 ENTER 60',252],['BITWISE_XOR','240 ENTER 60',204],
  ['BITWISE_NOT','15',240],['BITWISE_NEG','1',255],['SHIFT_LEFT_LOGICAL','129',2],['SHIFT_RIGHT_LOGICAL','3',1],
  ['SHIFT_RIGHT_ARITHMETIC','128',192],['SHIFT_LEFT_LOGICAL_N','49 ENTER 3',136],['SHIFT_RIGHT_LOGICAL_N','248 ENTER 4',15],
  ['ROTATE_LEFT_CIRCULAR','129',3],['ROTATE_RIGHT_CIRCULAR','1',128],['ROTATE_LEFT_CARRY','128',0],['ROTATE_RIGHT_CARRY','1',0],
  ['ROTATE_LEFT_CIRCULAR_N','18 ENTER 4',33],['ROTATE_RIGHT_CIRCULAR_N','18 ENTER 4',33],
  ['CLEAR_BIT','255 ENTER 7',127],['SET_BIT','0 ENTER 7',128],['GET_BIT','16 ENTER 4',1],['REVERSE_BIT','1',128],['COUNT_BIT','183',6],
  ['BASE_ADD','255 ENTER 2',1],['BASE_SUBT','1 ENTER 2',255],['BASE_MULT','16 ENTER 16',0],['BASE_DIV','17 ENTER 5',3],['BASE_DIV2','17 ENTER 5',2]
]) command(id,input,exact(value),{setup:baseSetup, checks: id === 'BASE_DIV2' ? ['Y contains the quotient 3.'] : []});
for (const [id,input,value] of [['ROTATE_LEFT_CARRY_N','128 ENTER 2',1],['ROTATE_RIGHT_CARRY_N','1 ENTER 2',128]]) command(id,input,exact(value),{setup:baseSetup});
for (const [id,expect] of [['CLEAR_CARRY_FLAG',0],['SET_CARRY_FLAG',1]]) add(`carry-${id}`,'BASE',`${byId[id].label}: read carry`,[...baseSetup,select(id),select('GET_CARRY_FLAG')],exact(expect),[id,'GET_CARRY_FLAG']);
command('GET_WORD_SIZE','8',exact(8),{setup:baseSetup,covers:['GET_WORD_SIZE','SET_WORD_SIZE']});
for (const [id,labelValue] of [['DEC','255'],['HEX','FF'],['OCT','377'],['BIN','11111111']]) add(`base-format-${id}`,'BASE',`Display 255 in ${id}`,[...baseSetup,'Enter 255.',select(id),'Read the digits in X (ignore grouping and the base/status label).',select('DEC')],null,[id],{checks:[`Before returning to DEC, X reads ${labelValue}.`,'Returning to DEC gives 255.']});

const statSetup = [select('STAT_CLEAR'),select('STAT_ALL_MODE'), 'Enter 2 ENTER 1.',select('STAT_PLUS'),'Enter 4 ENTER 2.',select('STAT_PLUS'),'Enter 6 ENTER 3.',select('STAT_PLUS')];
for (const [id,value,check] of [
  ['STAT_PLUS',3,''],['STAT_N',3,''],['STAT_REG_N',3,''],['STAT_REG_X',6,''],['STAT_REG_X2',14,''],['STAT_REG_Y',12,''],['STAT_REG_Y2',56,''],['STAT_REG_XY',28,''],
  ['STAT_SUM',6,'Y contains 12.'],['STAT_MEAN',2,'Y contains 4.'],['STAT_WEIGHTED_MEAN',ref.weightedX,'Y is approximately 4.6666666666667.'],
  ['STAT_SAMPLE_SDEV',1,'Y contains 2.'],['STAT_POP_SDEV',ref.sqrt23,'Y is approximately 1.6329931618555.'],['STAT_SAMPLE_COV',2,''],['STAT_POP_COV','1.3333333333333333333333333333333',''],
  ['STAT_REG_LN_X',ref.lnsum,''],['STAT_REG_LN_X2',ref.lnx2,''],['STAT_REG_LN_Y',ref.lnysum,''],['STAT_REG_LN_Y2',ref.lny2,''],
  ['STAT_REG_LN_XLN_Y',ref.lnxlny,''],['STAT_REG_XLN_Y',ref.xlny,''],['STAT_REG_YLN_X',ref.ylnx,'']
]) add(`stat-${id}`,'STAT',`${byId[id].label} for (1,2), (2,4), (3,6)`,[...statSetup,...(id==='STAT_PLUS'?[]:[select(id)])],approx(value),[id],{checks:check?[check]:[]});
add('stat-remove','STAT','Remove a data point',[...statSetup,'Enter 6 ENTER 3.',select('STAT_MINUS'),select('STAT_N')],exact(2),['STAT_MINUS']);
add('stat-empty','STAT','Clear statistics',[...statSetup,select('STAT_CLEAR'),select('STAT_N')],exact(0),['STAT_CLEAR']);
add('stat-linear-only','STAT','Linear-only statistics',[select('STAT_CLEAR'),select('STAT_LINEAR_MODE'),'Enter 2 ENTER 1.',select('STAT_PLUS'),'Enter 4 ENTER 2.',select('STAT_PLUS'),select('STAT_N')],exact(2),['STAT_LINEAR_MODE'],{checks:['LINΣ is marked selected; ALLΣ can be selected again.']});
for (const [id,input,value] of [['CFIT_SLOPE',null,2],['CFIT_INTERCEPT',null,0],['CFIT_CORRELATION',null,1],['CFIT_FORECAST_Y','4',8],['CFIT_FORECAST_X','8',4]]) add(`fit-${id}`,'STAT',`${byId[id].label} on y = 2x`,[...statSetup,select('CFIT_LINEAR'),...(input?[`Enter ${input}.`]:[]),select(id)],approx(value),[id,'CFIT_LINEAR']);
for (const [id,points,want] of [['CFIT_EXP',[[0,1],[1,2],[2,4]],8],['CFIT_POWER',[[1,2],[2,8],[3,18]],32]]) add(`fit-model-${id}`,'STAT',`${byId[id].label}: forecast an exact sequence`,[select('STAT_CLEAR'),select('STAT_ALL_MODE'),...points.flatMap(([x,y])=>[`Enter ${y} ENTER ${x}.`,select('STAT_PLUS')]),select(id),`Enter ${id==='CFIT_EXP'?3:4}.`,select('CFIT_FORECAST_Y')],approx(want),[id]);
add('fit-best','STAT','BEST chooses a fit',[...statSetup,select('CFIT_BEST'),'Enter 4.',select('CFIT_FORECAST_Y')],approx(8),['CFIT_BEST']);

// TVM cases replace the calculator's shared Finance variables.
const tvmSetup = [select('TVM_CLEAR'),select('TVM_SOLVER_RESET')];
add('tvm-mortgage','TVM','Mortgage payment',[...tvmSetup,'Enter 360.',select('TVM_N'),'Enter 6.5.',select('TVM_IYR'),'Enter 200000.',select('TVM_PV'),'Enter 0.',select('TVM_FV'),select('TVM_PMT')],approx(ref.mortgage),['TVM_N','TVM_IYR','TVM_PV','TVM_FV','TVM_PMT','TVM_CLEAR','TVM_SOLVER_RESET']);
add('tvm-savings','TVM','Annual compound growth',[...tvmSetup,'Enter 1.',select('TVM_PYR'),'Enter 1.',select('TVM_CYR'),'Enter 10.',select('TVM_N'),'Enter 5.',select('TVM_IYR'),'Enter 1000 (−).',select('TVM_PV'),'Enter 0.',select('TVM_PMT'),select('TVM_FV')],approx(ref.growth),['TVM_PYR','TVM_CYR','TVM_FV']);
add('tvm-zero-interest','TVM','Zero-interest payment',[...tvmSetup,'Enter 10.',select('TVM_N'),'Enter 0.',select('TVM_IYR'),'Enter 1000.',select('TVM_PV'),'Enter 0.',select('TVM_FV'),select('TVM_PMT')],exact(-100),['TVM_PMT']);
add('tvm-timing','TVM','Payment timing and solver settings',[...tvmSetup,select('TVM_BEGIN'),select('TVM_END'),'Enter 2.',select('TVM_IYR0'),'Enter 20.',select('TVM_IYR1'),'Enter 60.',select('TVM_ITER_MAX'),select('TVM_SOLVER_RESET')],null,['TVM_BEGIN','TVM_END','TVM_IYR0','TVM_IYR1','TVM_ITER_MAX','TVM_SOLVER_RESET'],{checks:['BEG and END mark the chosen timing without changing X.','IYR1, IYR2 and TMAX accept the values; 2ND with each softkey recalls the setting.','After RSTV, recalls show IYR1 = 0, IYR2 = 100 and TMAX = 50.']});

function dateInput(type, fields, enter = true) { return `Select ${menu(type)} to start a record, then type ${fields}${enter ? ' ENTER.' : '. Do not press ENTER: the following operation finishes this record without duplicating X.'} Use the comma key between fields and (−) for negative fields.`; }
const dateSetup = [select('EPOCH_UNIX')];
for (const [create,fields,want] of [['DATE_CREATE','2024,3,14','D{2024,3,14}'],['TIME_CREATE','12,47,0','T{12,47,0}'],['DATE_TIME_CREATE','2024,3,14,15,36,1','DT{2024,3,14,15,36,1}'],['OFFSET_CREATE','5,30','TZ{5,30}'],['OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0','DZ{2024,3,14,15,36,1,-7,0}'],['DURATION_CREATE','1,2,3,4','DR{1,2,3,4}'],['DAY_OF_WEEK_CREATE','4','DW{4}']]) add(`date-create-${create}`,'DATE',`Create ${want}`,[dateInput(create,fields)],text(want),[create]);
add('date-15000-days','DATE','Add 15,000 days to a date',[dateInput('DATE_CREATE','1987,1,17'),'Enter 15000 +.'],text('D{2028,2,11}'),[]);
add('date-subtract','DATE','Subtract dates',[dateInput('DATE_CREATE','2024,12,25'),dateInput('DATE_CREATE','2024,3,14',false),'Press −.'],exact(286));
add('time-subtract','DATE','Subtract times: answer is seconds',[dateInput('TIME_CREATE','12,47,0'),dateInput('TIME_CREATE','20,17,0',false),'Press −.'],exact(-27000),[]);
add('time-difference-duration','DATE','Display a time difference as a duration',[dateInput('TIME_CREATE','12,47,0'),dateInput('TIME_CREATE','20,17,0',false),'Press −.',select('SECONDS_TO_DURATION')],text('DR{0,-7,-30,0}'),['SECONDS_TO_DURATION']);
add('time-midnight','DATE','Time wraps at midnight',[dateInput('TIME_CREATE','23,59,59'),'Enter 2 +.'],text('T{0,0,1}'));
for (const [id,create,fields,want] of [
 ['DATE_EXTRACT_YEAR','DATE_CREATE','2024,3,14',2024],['DATE_EXTRACT_MONTH','DATE_CREATE','2024,3,14',3],['DATE_EXTRACT_DAY','DATE_CREATE','2024,3,14',14],
 ['IS_DATE_LEAP','DATE_CREATE','2024,3,14',1],['DATE_TO_EPOCH_SECONDS','DATE_CREATE','2024,3,14',1710374400],['DATE_TO_EPOCH_DAYS','DATE_CREATE','2024,3,14',19796],
 ['TIME_TO_SECONDS','TIME_CREATE','12,47,0',46020],['TIME_EXTRACT_HOUR','TIME_CREATE','12,47,0',12],['TIME_EXTRACT_MINUTE','TIME_CREATE','12,47,0',47],['TIME_EXTRACT_SECOND','TIME_CREATE','12,47,0',0],
 ['DATE_TIME_TO_EPOCH_SECONDS','DATE_TIME_CREATE','2024,3,14,15,36,1',1710430561],['DATE_TIME_TO_EPOCH_DAYS','DATE_TIME_CREATE','2024,3,14,0,0,0',19796],
 ['IS_DATE_TIME_LEAP','DATE_TIME_CREATE','2024,3,14,15,36,1',1],['OFFSET_TO_HOURS','OFFSET_CREATE','5,30','5.5'],['OFFSET_EXTRACT_HOUR','OFFSET_CREATE','5,30',5],['OFFSET_EXTRACT_MINUTE','OFFSET_CREATE','5,30',30],
 ['OFFSET_DATE_TIME_TO_EPOCH_SECONDS','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0',1710455761],['IS_OFFSET_DATE_TIME_LEAP','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0',1],
 ['DURATION_TO_SECONDS','DURATION_CREATE','1,2,3,4',93784],['DURATION_EXTRACT_DAY','DURATION_CREATE','1,2,3,4',1],['DURATION_EXTRACT_HOUR','DURATION_CREATE','1,2,3,4',2],['DURATION_EXTRACT_MINUTE','DURATION_CREATE','1,2,3,4',3],['DURATION_EXTRACT_SECOND','DURATION_CREATE','1,2,3,4',4],
 ['DAY_OF_WEEK_TO_ISO_DOW_NUMBER','DAY_OF_WEEK_CREATE','7',7],['DAY_OF_WEEK_TO_UNIX_DOW_NUMBER','DAY_OF_WEEK_CREATE','7',0]
]) add(`date-${id}`,'DATE',`${byId[id].path.slice(-2).join(' → ')} of ${fields}`,[...dateSetup,dateInput(create,fields),select(id)],exact(want),[id],{reference:'Gregorian calendar and integer second/day arithmetic; Unix epoch is 1970-01-01.'});
for (const [id,input,want] of [
 ['EPOCH_DAYS_TO_DATE','19796','D{2024,3,14}'],['EPOCH_SECONDS_TO_DATE','1710374400','D{2024,3,14}'],['SECONDS_TO_TIME','46020','T{12,47,0}'],
 ['EPOCH_DAYS_TO_DATE_TIME','19796','DT{2024,3,14,0,0,0}'],['EPOCH_SECONDS_TO_DATE_TIME','1710430561','DT{2024,3,14,15,36,1}'],
 ['HOURS_TO_OFFSET','5.5','TZ{5,30}'],['EPOCH_SECONDS_TO_OFFSET_DATE_TIME_UTC','1710455761','DZ{2024,3,14,22,36,1,0,0}'],
 ['DAYS_TO_DURATION','2','DR{2,0,0,0}'],['HOURS_TO_DURATION','2','DR{0,2,0,0}'],['MINUTES_TO_DURATION','90','DR{0,1,30,0}'],
 ['ISO_DOW_NUMBER_TO_DAY_OF_WEEK','7','DW{7}'],['UNIX_DOW_NUMBER_TO_DAY_OF_WEEK','0','DW{7}']
]) add(`date-inverse-${id}`,'DATE',`${byId[id].path.slice(-2).join(' → ')}: ${input}`,[...dateSetup,`Enter ${input}.`,select(id)],text(want),[id]);
for (const [id,create,fields,want] of [
 ['DATE_TO_DAY_OF_WEEK','DATE_CREATE','2024,3,14','DW{4}'],['DATE_TIME_TO_DAY_OF_WEEK','DATE_TIME_CREATE','2024,3,14,15,36,1','DW{4}'],['OFFSET_DATE_TIME_TO_DAY_OF_WEEK','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0','DW{4}'],
 ['DATE_TIME_EXTRACT_DATE','DATE_TIME_CREATE','2024,3,14,15,36,1','D{2024,3,14}'],['DATE_TIME_EXTRACT_TIME','DATE_TIME_CREATE','2024,3,14,15,36,1','T{15,36,1}'],
 ['OFFSET_DATE_TIME_EXTRACT_DATE','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0','D{2024,3,14}'],['OFFSET_DATE_TIME_EXTRACT_TIME','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0','T{15,36,1}'],
 ['OFFSET_DATE_TIME_EXTRACT_DATE_TIME','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0','DT{2024,3,14,15,36,1}'],['OFFSET_DATE_TIME_EXTRACT_OFFSET','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0','TZ{-7,0}'],
 ['DATE_EXTEND_TO_DATE_TIME','DATE_CREATE','2024,3,14','DT{2024,3,14,0,0,0}'],['DATE_TIME_SHRINK_TO_DATE','DATE_TIME_CREATE','2024,3,14,15,36,1','D{2024,3,14}'],
 ['DATE_TIME_EXTEND_TO_OFFSET_DATE_TIME','DATE_TIME_CREATE','2024,3,14,15,36,1','DZ{2024,3,14,15,36,1,0,0}'],['OFFSET_DATE_TIME_SHRINK_TO_DATE_TIME','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0','DT{2024,3,14,15,36,1}']
]) add(`date-object-${id}`,'DATE',`${byId[id].path.slice(-2).join(' → ')}: object result`,[dateInput(create,fields),select(id)],text(want),[id]);
add('date-split-dt','DATE','Split DateTime',[dateInput('DATE_TIME_CREATE','2024,3,14,15,36,1'),select('DATE_TIME_CUT_TO_DATE')],text('T{15,36,1}'),['DATE_TIME_CUT_TO_DATE'],{checks:['Y is D{2024,3,14}.']});
add('date-split-dz','DATE','Split ZonedDateTime',[dateInput('OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0'),select('OFFSET_DATE_TIME_CUT_TO_DATE_TIME')],text('TZ{-7,0}'),['OFFSET_DATE_TIME_CUT_TO_DATE_TIME'],{checks:['Y is DT{2024,3,14,15,36,1}.']});
add('date-link-dt','DATE','Join Date and Time',[dateInput('DATE_CREATE','2024,3,14'),dateInput('TIME_CREATE','15,36,1',false),select('DATE_LINK_TO_DATE_TIME')],text('DT{2024,3,14,15,36,1}'),['DATE_LINK_TO_DATE_TIME']);
add('date-link-dz','DATE','Join DateTime and TimeZone',[dateInput('DATE_TIME_CREATE','2024,3,14,15,36,1'),dateInput('OFFSET_CREATE','7 (−),0',false),select('DATE_TIME_LINK_TO_OFFSET_DATE_TIME')],text('DZ{2024,3,14,15,36,1,-7,0}'),['DATE_TIME_LINK_TO_OFFSET_DATE_TIME']);
add('date-zone-convert','DATE','Convert a zoned instant across midnight',[dateInput('OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,7 (−),0'),'Enter 13 ×.'],text('DZ{2024,3,15,11,36,1,13,0}'),['OFFSET_DATE_TIME_CONVERT_TO_TIME_ZONE']);
for (const [id,date] of [['EPOCH_UNIX','1970,1,1'],['EPOCH_NTP','1900,1,1'],['EPOCH_GPS','1980,1,6'],['EPOCH_TIOS','1997,1,1'],['EPOCH_Y2K','2000,1,1']]) add(`epoch-${id}`,'DATE',`${byId[id].label}: epoch origin`,[select(id),dateInput('DATE_CREATE',date),select('DATE_TO_EPOCH_SECONDS')],exact(0),[id]);
add('epoch-custom','DATE','Custom epoch',[dateInput('DATE_CREATE','2024,1,1'),select('EPOCH_SET_CUSTOM'),select('EPOCH_CUSTOM'),dateInput('DATE_CREATE','2024,1,2'),select('DATE_TO_EPOCH_SECONDS')],exact(86400),['EPOCH_CUSTOM','EPOCH_SET_CUSTOM']);
add('epoch-custom-read','DATE','Recall the custom epoch',[dateInput('DATE_CREATE','2024,1,1'),select('EPOCH_SET_CUSTOM'),select('EPOCH_GET_CUSTOM')],text('D{2024,1,1}'),['EPOCH_GET_CUSTOM']);

// Live clock checks are observations, not comparisons against the computer's clock.
add('clock-zones','DATE','Calculator zone and application zone',[
 'Before starting, write the TI-OS clock and its actual UTC offset in the notes. Keep the TI-OS clock set to the correct local time.',
 'Enter that UTC offset in hours (for example -7); select CTZ. Enter the same offset and select ATZ.',
 select('GET_CLOCK_TIME_ZONE'), 'Check the recalled CTZ, then select ATZ? and check it.',
 select('GET_NOW_TIME'),select('GET_NOW_DATE'),select('GET_NOW_OFFSET_DATE_TIME'),select('GET_NOW_OFFSET_DATE_TIME_UTC'),
 'Change only ATZ to an offset 2 hours later, within -12..+14; read NOWT and NWDZ again. Restore your original ATZ.'
],null,['SET_CLOCK_TIME_ZONE','GET_CLOCK_TIME_ZONE','SET_APP_TIME_ZONE','GET_APP_TIME_ZONE','GET_NOW_TIME','GET_NOW_DATE','GET_NOW_OFFSET_DATE_TIME','GET_NOW_OFFSET_DATE_TIME_UTC'],{
 checks:['With CTZ = ATZ, NOWT/NOWD/NWDZ agree with the calculator clock (allow the elapsed testing time).','NWUT represents the same instant in UTC, including any date rollover.','Changing ATZ advances the app status bar and NOWT/NWDZ by 2 hours; the TI-OS clock stays unchanged.'] });
add('clock-epoch-seconds','DATE','NOW advances in seconds',[select('EPOCH_UNIX'),select('GET_NOW'),'Write the value in the notes. Wait approximately 10 seconds.',select('GET_NOW')],null,['GET_NOW'],{checks:['The second NOW is about 10 seconds greater than the first, allowing time spent pressing keys.']});
add('clock-set','DATE','SETC writes the system clock',[
 'Optional: this check changes the TI-OS clock. Record its current value first; skip if you want to preserve it.',
 'Keep CTZ at your real local UTC offset. Create a ZonedDateTime for an agreed test instant with an explicit offset.',select('SET_CLOCK'),
 'Exit RPNCE and inspect the TI-OS clock. Reopen RPNCE and inspect NOWT/NWDZ/NWUT. Restore the correct system time after testing.'
],null,['SET_CLOCK'],{checks:['The TI-OS clock holds the test instant expressed in CTZ.','The application results represent that same instant in ATZ/UTC, respectively.']});

add('units-add','UNIT','Add compatible lengths', ['Enter 6.',select('UNIT_FOOT'),'Enter 2.',select('UNIT_INCH'),'Press +.'],approx('6.1666666666666666666666666666667'),[],{checks:['The result unit is ft.']});
add('units-ratio','UNIT','Divide compatible lengths', ['Enter 6.',select('UNIT_FOOT'),'Enter 2.',select('UNIT_INCH'),'Press ÷.'],approx(36),[],{checks:['The answer is a plain number with no unit.']});
add('units-energy-change','UNIT','Percent change: 2 kWh to 2000 kcal',['Enter 2.',select('UNIT_KILO_WATT_HOUR'),'Enter 2000.',select('UNIT_KILO_CALORIE'),select('PERCENT_CHANGE')],approx(ref.energychange),['PERCENT_CHANGE'],{checks:['Y remains 2 kWh and X is a plain percentage.']});
add('units-base','UNIT','UBAS converts to the base unit',['Enter 12.',select('UNIT_FOOT'),select('UNIT_BASE')],exact('3.6576'),['UNIT_BASE'],{checks:['The answer is labelled m.']});
add('units-value','UNIT','UVAL removes the unit',['Enter 12.',select('UNIT_FOOT'),select('UNIT_VALUE')],exact(12),['UNIT_VALUE'],{checks:['X has no unit.']});

// Physical keys, stack lift, registers, display, and saved-state checks.
add('keys-arithmetic','Keyboard','Four operations and RPN order',['Enter 8 ENTER 2 − 3 × 4 ÷ 1 +.'],exact('5.5'),[]);
for (const [id,title,input,expected] of [
 ['power','Power of a negative integer','8 (−) ENTER 3 ^',exact(-512)],['reciprocal','Reciprocal','4 x⁻¹',exact('.25')],['square','Square','12 x²',exact(144)],
 ['sqrt','Square root','2 then 2ND x²',approx(ref.sqrt2)],['log','Common logarithm','1000 LOG',exact(3)],['ln','Natural logarithm','2 LN',approx(ref.ln2)],
 ['ten-power','10^X','3 then 2ND LOG',exact(1000)],['exp','e^X','1 then 2ND LN',approx(ref.e)],['sin','SIN in RAD','0 SIN',exact(0)],['cos','COS in RAD','0 COS',exact(1)],['tan','TAN in RAD','0 TAN',exact(0)],
 ['asin','ASIN in RAD','1 then 2ND SIN',approx('1.5707963267948966192313216916398')],['acos','ACOS in RAD','0 then 2ND COS',approx('1.5707963267948966192313216916398')],['atan','ATAN in RAD','1 then 2ND TAN',approx('.78539816339744830961566084581988')],
 ['pi','PI constant','2ND ^',approx(ref.pi)],['e','e constant','2ND ÷',approx(ref.e)]
]) add(`key-${id}`,'Keyboard',title,[`Press ${input}.`],expected);
add('entry-edit','Keyboard','Edit a grouped number',['Type 1234567.89 without ENTER. Check the automatic grouping.','Press DEL twice, then 5 ENTER.'],exact('1234567.5'),[],{checks:['During entry, X read 1,234,567.89 before DEL.']});
add('entry-exponent','Keyboard','Exponent entry and sign',['Type 1.25 , (−) 9 ENTER. The comma key is EE outside DATE records.'],exact('1.25E-9'));
add('entry-lastx','Keyboard','LASTX after arithmetic',['Enter 8 ENTER 3 ×. Press 2ND (−) (ANS).'],exact(3));
add('entry-clear','Keyboard','Clear an unfinished entry',['Enter 9 ENTER. Type 123, then CLEAR. Type 4 +.'],exact(13));
for (const [id,steps,want,checks] of [
 ['STACK_DUP',['Enter 1 ENTER 2.',select('STACK_DUP')],2,['Y contains 2 and Z contains 1.']],
 ['STACK_ROLL_UP',['Enter 1 ENTER 2 ENTER 3 ENTER 4.',select('STACK_ROLL_UP')],1,['Y = 4, Z = 3, T = 2.']],
 ['STACK_ROLL_DOWN',['Enter 1 ENTER 2 ENTER 3 ENTER 4.',select('STACK_ROLL_DOWN')],3,['Y = 2, Z = 1, T = 4.']],
 ['STACK_DROP',['Enter 1 ENTER 2 ENTER 3 ENTER 4.',select('STACK_DROP')],3,['Y = 2, Z = 1, T = 1.']],
 ['STACK_EXCHANGE_XY',['Enter 1 ENTER 2.',select('STACK_EXCHANGE_XY')],1,['Y contains 2.']],
 ['STACK_EXCHANGE_REG',['Enter 12 STO→ MATH. Enter 34.',select('STACK_EXCHANGE_REG'),'At the register prompt press MATH (A).'],12,['Recalling A gives 34.']]
]) add(`stack-${id}`,'STK',byId[id].label,steps,exact(want),[id],{checks});
add('stack-top-copy','STK','The top level repeats after a drop',['Enter 1 ENTER 2 ENTER 3 ENTER 4. Press + + + +.'],exact(11),[]);
add('register-store','Registers','Store and recall A',['This test replaces A. Enter 123.45 STO→ MATH. Press CLEAR. Press 2ND STO→ MATH.'],exact('123.45'),[]);
add('register-arithmetic','Registers','STO arithmetic',['This test replaces A. Enter 100 STO→ MATH. Enter 25 STO→ + MATH. Press CLEAR, then 2ND STO→ MATH.'],exact(125));
add('register-indirect','Registers','Indirect register A',['This test replaces i and A. Enter 1 STO→ . ; enter 77 STO→ 2ND . ; press CLEAR; press 2ND STO→ MATH.'],exact(77));
add('register-extra','Registers','Extra register 041',['This test replaces i and R041. Enter 60.',select('SET_REG_SIZE'),'Enter 41 STO→ . ; enter 123 STO→ 2ND . ; press CLEAR; press 2ND STO→ 2ND .'],exact(123),['SET_REG_SIZE']);
add('register-quantity','Registers','A retains a measurement',['This test replaces A. Enter 12.',select('UNIT_FOOT'),'Press STO→ MATH. Press CLEAR, then 2ND STO→ MATH.',select('UNIT_METER')],exact('3.6576'),[],{checks:['A was recalled as 12 ft; the converted X is labelled m.']});
add('register-vars','Registers','VARS and LASTX',['Enter 123 STO→ MATH. Open VARS, locate A, select it with ENTER.','Open VARS again, scroll to the end; inspect the seven TVM entries and LASTX. Close with ON.'],null,[],{checks:['Selecting A recalls 123.','All seven TVM values are present, including zeros.','LASTX is the last item, and ON closes without recalling an item.']});
add('register-date','Registers','A retains a Date',['This test replaces A.',dateInput('DATE_CREATE','2024,3,14'),'Press STO→ MATH. Press CLEAR, then 2ND STO→ MATH.'],text('D{2024,3,14}'));
command('GET_REG_SIZE','60',exact(60),{setup:['Enter 60.',select('SET_REG_SIZE')],covers:['SET_REG_SIZE','GET_REG_SIZE']});
command('GET_STACK_SIZE','12',exact(12),{setup:['Enter 12.',select('SET_STACK_SIZE')],covers:['SET_STACK_SIZE','GET_STACK_SIZE'],suffix:['Record the query value before restoring the stack depth to 4. After recording, enter 4 and select SSIZ.']});
add('display-formats','MODE','FIX, SCI, ENG and SHOW',['Enter 1 ENTER 7 ÷.',select('FIX'),'Press 4. Inspect X.',select('SCI'),'Press 4. Inspect X.',select('ENG'),'Press 4. Inspect X. Press 2ND ENTRY.'],null,['FIX','SCI','ENG'],{checks:['FIX shows 0.1429; SCI shows about 1.4286E-1; ENG shows about 142.86E-3.','SHOW gives about 0.14285714285714; changing formats preserved the value.']});
add('display-angle','MODE','DEG and RAD',[select('DEG'),'Enter 180 SIN. Check X.',select('RAD'),'Enter 0 COS.'],exact(1),['DEG','RAD'],{checks:['180 SIN in DEG gave exactly zero.']});
add('display-decimal','MODE','Comma decimal throughout the interface',[select('DECIMAL_COMMA'),'Type 1234567.89 using the physical decimal key; inspect the unfinished entry. Press ENTER STO→ MATH.','Inspect VARS and SHOW for A. Close VARS/SHOW before returning to the calculator.',select('DECIMAL_POINT')],null,['DECIMAL_COMMA','DECIMAL_POINT'],{checks:['Entry, result, VARS and SHOW use 1.234.567,89 (allow trailing zeros).','Returning to period decimal shows 1,234,567.89 without changing the value.']});
add('display-date-formats','MODE','Raw and readable DATE display',[dateInput('DATE_CREATE','2024,3,14'),select('FORMAT_RECORD_STRING'),'Inspect X, then select the raw format.',select('FORMAT_RECORD_RAW')],text('D{2024,3,14}'),['FORMAT_RECORD_RAW','FORMAT_RECORD_STRING'],{checks:['Readable format showed 2024-03-14.']});
add('display-tints','MODE','Eight color themes',[select('SCREEN_TINT'),'Repeat TINT until you return to your starting theme. Check each theme with several stack values and an error message.'],null,['SCREEN_TINT'],{checks:['LCD, LCD+, GRAY, INVERTED, AYU DARK, MONOKAI, PASTEL and COMMODORE are all reachable.','Numbers, units, warnings and menus remain readable on the actual LCD.']});
add('display-brightness','Keyboard','Brightness keys during entry',['Note your current brightness. Type 12345 without ENTER. Press 2ND ↑ a few times, then 2ND ↓.','Confirm the entry is preserved; press ENTER. Restore a comfortable brightness.'],exact(12345),[],{checks:['Brightness changes in both directions and the display remains readable.']});
add('display-messages','CLR','Warning size and clearing messages',[select('CLEAR_STACK'),'Inspect “Stack cleared”. Press ↓ to reach CLR row 2 BEFORE creating the next error.','Enter 1 ENTER 0 ÷; inspect the error. Without navigating away, press Y= (CLD).'],null,['CLEAR_STACK','CLEAR_DISPLAY'],{checks:['Both notices use the same enlarged message font; menu arrows and status bar retain their normal sizes.','CLD clears the message without changing stack values.']});
command('CLEAR_X','42',exact(0));
add('clear-stack','CLR','Clear all stack levels',['Enter 1 ENTER 2 ENTER 3 ENTER 4.',select('CLEAR_STACK')],exact(0),['CLEAR_STACK'],{checks:['All four visible levels are zero.']});
add('clear-registers','CLR','Clear ordinary registers',['This test clears all ordinary registers. Save anything you need first. Enter 99 STO→ MATH.',select('CLEAR_REGS'),'Press 2ND STO→ MATH.'],exact(0),['CLEAR_REGS']);
add('clear-stat','CLR','Clear statistics from CLR',[...statSetup,select('CLEAR_STAT'),select('STAT_N')],exact(0),['CLEAR_STAT']);
add('clear-tvm','CLR','Clear shared TVM values from CLR',[select('CLEAR_TVM'),'Open VARS and inspect the TVM entries. Close with ON.'],null,['CLEAR_TVM'],{checks:['N, I%YR, PV, PMT and FV are 0; P/YR and C/YR are 12; END is selected.']});
add('help-pages','HELP','Calculator reference pages',[select('HELP'),'Use ↑/↓ to visit the pages, including About. Press ON. Open HELP again with 2ND 0; close with ENTER.'],null,['HELP'],{checks:['Pages are legible, navigation works, and About gives version 0.7.0 and the repository link.','Both exits return to the stack without changing it.']});
add('key-shortcuts','Keyboard','Menu shortcuts',['Try MODE, STAT, X,T,θ,n, APPS and PRGM, returning to HOME with MATH each time.','Try 2ND + (MEM), 2ND STAT (LIST), and 2ND 0 (CATALOG). Return with ON.'],null,[],{checks:['The first keys open MODE, STAT, UNIT, BASE and HYP, respectively.','The 2ND keys open CLR, STK and HELP, respectively.']});
add('persist-state','Persistence','Save, quit and reopen',[
 'This check replaces A, B and the active stack. Enter 12.',select('UNIT_FOOT'),'Press STO→ MATH (A).',
 dateInput('DATE_CREATE','2024,3,14'),'Press STO→ APPS (B). Choose a distinctive TINT and comma decimal mode. Note the selected settings.',
 'Quit with 2ND MODE. Relaunch the program without deleting its appvars. Recall A and B and inspect the stack, VARS, tint and decimal mode.'
],null,[],{checks:['A retains 12 ft, B retains the Date object, and the saved stack is restored.','Tint and decimal mode survive the restart; no load error is shown.']});

// Boundary expectations follow the documented range, rather than today's buggy output.
for (const [id,title,steps,expected,known] of [
 ['low-entry','Entry at the lower exponent limit',['Enter 1 EE (−) 99 ENTER.'],exact('1E-99'),false],
 ['high-entry','Entry at the upper exponent limit',['Enter 1 EE 99 ENTER.'],exact('1E99'),false],
 ['expm1-smallest','E^X- keeps a tiny nonzero value',['Enter 1 EE (−) 99.',select('EXP_MINUS_ONE')],approx('1E-99'),false],
 ['log1p-smallest','LN1+ keeps a tiny nonzero value',['Enter 1 EE (−) 99.',select('LN_ONE_PLUS')],approx('1E-99'),false],
 ['factorial-69','Largest supported integer factorial',['Enter 69.',select('FACTORIAL')],approx(ref.fact69),false],
 ['date-min','Earliest date epoch seconds',[...dateSetup,dateInput('DATE_CREATE','1,1,1'),select('DATE_TO_EPOCH_SECONDS')],exact(-62135596800),false],
 ['date-max','Latest date epoch seconds',[...dateSetup,dateInput('DATE_CREATE','9999,12,31'),select('DATE_TO_EPOCH_SECONDS')],exact(253402214400),false]
]) add(`boundary-${id}`,'Boundaries',title,steps,expected,[],{knownIssue:known,focus:known?'regression':'boundary',reference:'Independent decimal / Gregorian reference.'});
for (const [id,title,steps,message] of [
 ['divide-zero','Division by zero',['Enter 1 ENTER 0 ÷.'],'Err: Division By 0'],
 ['negative-sqrt','Square root of a negative real',['Enter 1 (−), then 2ND x².'],'Err: Domain'],
 ['ln-zero','Logarithm at zero',['Enter 0 LN.'],'Err: Domain'],
 ['acosh-domain','ACOSH below one',['Enter .5.',select('ACOSH')],'Err: Domain'],
 ['atanh-endpoint','ATANH at one',['Enter 1.',select('ATANH')],'Err: Domain'],
 ['root-even','Even root of a negative real',['Enter 8 (−) ENTER 2.',select('XROOT_Y')],'Err: Domain'],
 ['factorial-70','Factorial above the supported range',['Enter 70.',select('FACTORIAL')],'Err: Domain'],
 ['unit-mismatch','Conversion between incompatible units',['Enter 1.',select('UNIT_METER'),select('UNIT_KILO_GRAM')],'Err: Unit Mismatch'],
 ['unit-product','Product of two quantities',['Enter 2.',select('UNIT_METER'),'Enter 3.',select('UNIT_METER'),'Press ×.'],'Err: Data Type'],
 ['unit-temp-arithmetic','Temperature arithmetic is refused',['Enter 10.',select('UNIT_CELSIUS'),'Enter 2 +.'],'Err: Data Type'],
 ['date-invalid','Invalid leap date',['Select DATE → D → D{}; type 2023,2,29 ENTER.'],'Err: Invalid']
]) add(`error-${id}`,'Boundaries',title,steps,error(message),[],{checks:['After noting the message, CLEAR dismisses/cancels the bad entry and normal calculation is still possible. Enter the original message above.']});
add('stat-all-sums','STAT','ALLΣ retains logarithmic sums',[
 select('STAT_CLEAR'),select('STAT_ALL_MODE'),'Enter 2 ENTER 2.',select('STAT_PLUS'),select('STAT_REG_LN_X')
],approx(ref.ln2),['STAT_ALL_MODE']);
add('fit-logarithmic','STAT','LOGF: forecast a logarithmic sequence',[
 select('STAT_CLEAR'),select('STAT_ALL_MODE'),'Enter 1 ENTER 1.',select('STAT_PLUS'),
 'Enter 3 ENTER 10.',select('STAT_PLUS'),'Enter 5 ENTER 100.',select('STAT_PLUS'),
 select('CFIT_LOG'),'Enter 1000.',select('CFIT_FORECAST_Y')
],approx(7),['CFIT_LOG']);
for (const [id,create,fields,want] of [
 ['DATE_CONVERT_TO_TIME_ZONE','DATE_CREATE','2024,3,14','DZ{2024,3,14,9,0,0,9,0}'],
 ['DATE_TIME_CONVERT_TO_TIME_ZONE','DATE_TIME_CREATE','2024,3,14,15,36,1','DZ{2024,3,15,0,36,1,9,0}']
]) add(`date-zone-${id}`,'DATE',`${byId[id].path.slice(-2).join(' → ')}: UTC to +09:00`,[
 dateInput(create,fields),'Enter 9.',select(id)
],text(want),[id]);
add('date-zoned-epoch-days','DATE','ZonedDateTime epoch days',[
 ...dateSetup,dateInput('OFFSET_DATE_TIME_CREATE','2024,3,14,0,0,0,0,0'),select('OFFSET_DATE_TIME_TO_EPOCH_DAYS')
],exact(19796),['OFFSET_DATE_TIME_TO_EPOCH_DAYS']);
add('date-zoned-from-days','DATE','Epoch days to UTC ZonedDateTime',[
 ...dateSetup,'Enter 19796.',select('EPOCH_DAYS_TO_OFFSET_DATE_TIME')
],text('DZ{2024,3,14,0,0,0,0,0}'),['EPOCH_DAYS_TO_OFFSET_DATE_TIME']);
add('date-zoned-from-app-seconds','DATE','Epoch seconds in the application zone',[
 ...dateSetup,'Note your original ATZ. Enter 9.',select('SET_APP_TIME_ZONE'),
 'Enter 1710455761.',select('EPOCH_SECONDS_TO_OFFSET_DATE_TIME_APP'),
 'Record the object shown; restore your original ATZ after this case.'
],text('DZ{2024,3,15,7,36,1,9,0}'),['EPOCH_SECONDS_TO_OFFSET_DATE_TIME_APP']);
for (const [id,create,fields] of [
 ['DATE_SHRINK_TO_NOTHING','DATE_CREATE','2024,3,14'],['DATE_CUT_TO_NOTHING','DATE_CREATE','2024,3,14'],
 ['OFFSET_DATE_TIME_EXTEND_TO_NOTHING','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,0,0'],
 ['OFFSET_DATE_TIME_LINK_TO_NOTHING','OFFSET_DATE_TIME_CREATE','2024,3,14,15,36,1,0,0']
]) add(`date-unavailable-cast-${id}`,'DATE',`${byId[id].path.slice(-2).join(' → ')} rejects this object`,[
 dateInput(create,fields),select(id)
],error('Err: Data Type'),[id]);

const expanded = typeof module !== 'undefined' && module.exports ? require('./questions.js') : root.HardwareQuestions;
cases.push(...expanded.cases);
for (const test of cases) {
  if (test.kind === 'navigation') test.focus = 'navigation';
  else if (test.knownIssue) test.focus = 'regression';
  else if (test.expected?.kind === 'error' && test.focus === 'normal') test.focus = 'domain';
}
const catalog = {
 revision: 'hardware-0.7.0-6', version: '0.7.0', menuDigest: source.menuDigest,
 reference: 'Exact integer and Gregorian references; basic mpmath references at 70 decimal digits and expanded references at 180 digits. Unit references reuse the exact definitions in tools/genunits.py and are evaluated separately. Most approximate checks allow 5E-11 relative error and zero absolute error; iterative-rate, near-pole and cancellation cases state their own tolerances. Use SHOW for numeric results, especially scientific notation. Passing means the reported observations satisfy these cases; it is not proof of a complete hardware audit.',
 baseline: [
  'Use a calculator with the current RPNCE program and matching resource appvar. Copy any stack/register values you want to keep: these exercises replace them. TVM cases replace the shared TI-OS Finance values.',
  'Before EACH case, cancel entry/prompts with ON or CLEAR as needed. Press APPS then Y= for DEC. Press MODE then TRACE for RAD. Press MODE then Y=, then . then 2 for FIX all digits.',
  'Press MODE, ↓ three times, then Y= for raw DATE objects. Press MODE, ↓ three times, then ZOOM for period decimal. Enter 4; press MODE, ↓ twice, TRACE for stack depth 4.',
  'Press CLEAR twice to clear the stack. Follow the case-specific setup and steps after this baseline. A case that changes settings does not set up the next case.',
  'Named menu selections have a physical-key route below the steps. All routes begin with MATH (HOME). EE means the comma key; (−) is the sign key, distinct from subtraction.',
  'For a numeric answer, inspect any unit/Y-register checks FIRST, then press 2ND ENTER (ENTRY / SHOW) to read all stored digits. Enter that value, using E for its exponent, without the unit. For an error, enter its exact message. Raw DATE answers include their type and braces.'
 ],
 commands: source.commands.map(c => ({...c, available:!unavailable.includes(c.id)})), groups: source.groups, cases, requirements: expanded.requirements,
 gaps: [
  'Commands shown under “Functional gaps” have navigation coverage but no functional case yet. They must not be counted as functionally verified.',
  'This pool samples numeric classes and boundaries; it is not exhaustive. All-pairs unit limits, arbitrary noisy/nonlinear fits, every possible cash-flow convergence path, low-battery/power interruption, damaged appvars, and other ROM versions still need additional cases.',
  'Underflow requires an explicit error. Cases list any specifically accepted precision or intermediate-range error; an arbitrary error is not a pass. Behavior of Σ− on an empty dataset remains unspecified.',
  'This tool checks user-reported results. It cannot observe the calculator or establish which binary is installed. A build identifier is required so reports can be matched to the tested program.'
 ]
};
if (typeof module !== 'undefined' && module.exports) module.exports = catalog;
else root.HardwareCatalog = catalog;
})(globalThis);
