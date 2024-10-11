
export const MgKdb = {}

MgKdb.chk_guid = function(val) {
  return val // TODO
}

MgKdb.chk_is_int8 = function(val) {
  if (val === undefined || val === null) return 0
  if (!Number.isInteger(val)) throw new TypeError("'type")
  return val
}

MgKdb.chk_is_int16 = function(val) {
  if (val === undefined || val === null) return KdbConstants.NULL_SHORT
  if (!Number.isInteger(val)) throw new TypeError("'type")
  return val
}

MgKdb.chk_is_int32 = function(val) {
  if (val === undefined || val === null) return KdbConstants.NULL_INT
  if (!Number.isInteger(val)) throw new TypeError("'type")
  return val
}

MgKdb.chk_is_bigint = function(val) {
  if (val === undefined || val === null) return KdbConstants.NULL_LONG
  if (typeof(val) !== 'bigint') throw new TypeError("'type")
  return val
}

MgKdb.chk_is_float = function(val) {
  if (val === undefined || val === null || isNaN(val)) return NaN
  if (typeof(val) === 'number') return val
  throw new TypeError("'type")
}

MgKdb.chk_is_string = function(val) {
  if (val === undefined || val === null) return ''
  if (typeof(val) === 'string') return val
  throw new TypeError("'type")
}

MgKdb.int_to_0N_str = function(val, cfg, char) {
  if (cfg.NULL_VALUE === val) return `0N${char}`
  if (cfg.POS_INF_VALUE === val) return `0W${char}`
  if (cfg.NEG_INF_VALUE === val) return `-0W${char}`
  return null
}

MgKdb.fp_to_0N_str = function(val, char) {
  if (isNaN(val)) return `0N${char}`
  if (Number.POSITIVE_INFINITY === val) return `0W${char}`
  if (Number.NEGATIVE_INFINITY === val) return `-0W${char}`
  return null
}


const KdbConstants = {
  i16: {
    NULL_VALUE:-32768,
    POS_INF_VALUE: 32767,
    NEG_INF_VALUE: -32767,
  },
  i32: {
    NULL_VALUE: -2147483648,
    POS_INF_VALUE: 2147483647,
    NEG_INF_VALUE: -2147483647,
  },
  i64: {
    NULL_VALUE: -9223372036854775808n,
    POS_INF_VALUE: 9223372036854775807n,
    NEG_INF_VALUE: -9223372036854775807n,
  },
}

const MgFormat = {
  NUM_FMT_2D: new Intl.NumberFormat('en-GB', {minimumIntegerDigits: 2,}),
  NUM_FMT_3D: new Intl.NumberFormat('en-GB', {minimumIntegerDigits: 3,}),
  NUM_FMT_4D: new Intl.NumberFormat('en-GB', {minimumIntegerDigits: 3, useGrouping: false }),
  NUM_FMT_9D: new Intl.NumberFormat('en-GB', {minimumIntegerDigits: 9, useGrouping: false }),
}

export const KdbTimeUtil = {

  NANOS_IN_DAY: 86400000000000n,
  NANOS_IN_HOUR: 86400000000000n / 24n,
  NANOS_IN_MINUTE: 60000000000n,
  NANOS_IN_SECOND:  1000000000n,

  MILLIS_IN_SECOND: 1000,
  MILLIS_IN_MINUTE: 60 * 1000,
  MILLIS_IN_HOUR: 60 * 60 * 1000,
  MILLIS_IN_DAY: 24 * 60 * 60 * 1000,

  DAYS_SINCE_0AD: 730425,
  DAYS_SINCE_EPOCH: 10957,
  MILLIS_SINCE_EPOCH: 10957 * 24 * 60 * 60 * 1000,

  Timestamp: {
    nanosFromJsDate: function(date) {
      var nanos = BigInt(date.getTime() - KdbTimeUtil.MILLIS_SINCE_EPOCH)
      nanos *= 1000000n
      return nanos
    },
    atomFromJsDate: function(date) {
      const nanos = KdbTimeUtil.Timestamp.nanosFromJsDate(date)
      return new KdbTimestampAtom(nanos)
    },
    toJsDate: function(nanos) {
      let millis = Number(nanos / 1000000n)
      return new Date(millis + KdbTimeUtil.MILLIS_SINCE_EPOCH)
    },
    toTimeComponent: function(nanos) {
      nanos = nanos % KdbTimeUtil.NANOS_IN_DAY
      const h = nanos / KdbTimeUtil.NANOS_IN_HOUR
      const m = (nanos % KdbTimeUtil.NANOS_IN_HOUR) / KdbTimeUtil.NANOS_IN_MINUTE
      const s = (nanos % KdbTimeUtil.NANOS_IN_MINUTE) / KdbTimeUtil.NANOS_IN_SECOND
      const n = nanos % KdbTimeUtil.NANOS_IN_SECOND;

      const F2 = MgFormat.NUM_FMT_2D
      const F9 = MgFormat.NUM_FMT_9D
      return `${F2.format(h)}:${F2.format(m)}:${F2.format(s)}.${F9.format(n)}`
    },
    toDateComponent: function(nanos) {
      const days = Number(nanos / KdbTimeUtil.NANOS_IN_DAY)
      return KdbTimeUtil.Date.toString(days)
    },
    toString: function(nanos, suffix) {
      const nil = MgKdb.int_to_0N_str(nanos, KdbConstants.i64, suffix ? 'p' : '')
      if (nil) return nil
      const date = KdbTimeUtil.Timestamp.toDateComponent(nanos)
      const time = KdbTimeUtil.Timestamp.toTimeComponent(nanos)
      return `${date}D${time}`
    }
  },

  Month: {
    toJsDate: function(months) {
      const y = 2000 + Math.floor(months / 12)
      const m = months % 12
      return new Date(Date.UTC(y, m, 1))
    },
    toString: function(months, suffix) {
      const nil = MgKdb.int_to_0N_str(months, KdbConstants.i32, suffix ? 'm' : '')
      if (nil) return nil

      const F4 = MgFormat.NUM_FMT_4D
      const F2 = MgFormat.NUM_FMT_2D
      const yrs = 2000 + Math.floor(months / 12)
      const mth = 1 + months % 12
      return `${F4.format(yrs)}.${F2.format(mth)}${suffix ? 'm' : ''}`
    }
  },

  Date: {
    daysFromJsDate: function(date) {
      var days = Math.floor(date.getTime() / KdbTimeUtil.MILLIS_IN_DAY)
      days -= KdbTimeUtil.DAYS_SINCE_EPOCH
      return days
    },
    atomFromJsDate: function(date) {
      return new KdbDateAtom(KdbTimeUtil.Date.daysFromJsDate(date))
    },
    toJsDate: function(days) {
      days += KdbTimeUtil.DAYS_SINCE_EPOCH
      return new Date(days * KdbTimeUtil.MILLIS_IN_DAY)
    },
    toString: function(date, suffix) {
      const nil = MgKdb.int_to_0N_str(date, KdbConstants.i32, suffix ? 'd' : '')
      if (nil) return nil
      // C.f. http://web.archive.org/web/20170507133619/https://alcor.concordia.ca/~gpkatch/gdate-algorithm.html

      var g, y, ddd, mi, mm, dd
      g = KdbTimeUtil.DAYS_SINCE_0AD + date
	    y = Math.floor((10000 * g + 14780) / 3652425)
      ddd = g - (365 * y + Math.floor(y/4) - Math.floor(y/100) + Math.floor(y/400))
      if (ddd < 0) {
        y = y - 1;
        ddd = g - (365 * y + Math.floor(y/4) - Math.floor(y/100) + Math.floor(y/400))
      }
      mi = Math.floor((100*ddd + 52) / 3060)
      mm = (mi + 2) % 12 + 1
      y = y + Math.floor((mi + 2) / 12)
      dd = ddd - Math.floor((mi * 306 + 5) / 10) + 1

      const F4 = MgFormat.NUM_FMT_4D
      const F2 = MgFormat.NUM_FMT_2D

      return `${F4.format(y)}.${F2.format(mm)}.${F2.format(dd)}`
    }
  },

  Timespan: {
    toJsDate: function(nanos) {
      return new Date(Number(nanos / 1000000n))
    },
    toString: function(nanos, suffix) {
      const nil = MgKdb.int_to_0N_str(nanos, KdbConstants.i64, suffix ? 'n' : '')
      if (null !== nil) {
        return nil
      }

      var d, h, m, s, n;
      d = nanos / KdbTimeUtil.NANOS_IN_DAY
      nanos = nanos % KdbTimeUtil.NANOS_IN_DAY
      h = nanos / KdbTimeUtil.NANOS_IN_HOUR
      m = (nanos % KdbTimeUtil.NANOS_IN_HOUR) / KdbTimeUtil.NANOS_IN_MINUTE
      s = (nanos % KdbTimeUtil.NANOS_IN_MINUTE) / KdbTimeUtil.NANOS_IN_SECOND
      n = nanos % KdbTimeUtil.NANOS_IN_SECOND;

      const F2 = MgFormat.NUM_FMT_2D
      const F9 = MgFormat.NUM_FMT_9D
      return `${d}D${F2.format(h)}:${F2.format(m)}:${F2.format(s)}.${F9.format(n)}`
    }
  },

  Minute: {
    toJsDate: function(mins) {
      return new Date(mins * 60 * 1000)
    },
    toString: function(mins, suffix) {
      const nil = MgKdb.int_to_0N_str(mins, KdbConstants.i32, suffix ? 'u' : '')
      if (nil) return nil
      const hours = Math.floor(mins / 60)
      mins = mins % 60
      const F2 = MgFormat.NUM_FMT_2D
      return `${F2.format(hours)}:${F2.format(mins)}` 
    }
  },

  Second: {
    toJsDate: function(secs) {
      return new Date(secs * 1000)
    },
    toString: function(secs, suffix) {
      const nil = MgKdb.int_to_0N_str(secs, KdbConstants.i32, suffix ? 'v' : '')
      if (nil) return nil
      const hours = Math.floor(secs / 3600);
      secs -= hours * 3600;
      const mins = Math.floor(secs / 60);
      secs -= mins * 60;
      const F2 = MgFormat.NUM_FMT_2D
      return `${F2.format(hours)}:${F2.format(mins)}:${F2.format(secs)}`
    }
  },

  Time: {
    millisFromJsDate: function(date) {
      var time
      time = date.getUTCHours() * KdbTimeUtil.MILLIS_IN_HOUR
      time += date.getUTCMinutes() * KdbTimeUtil.MILLIS_IN_MINUTE
      time += date.getUTCSeconds() * KdbTimeUtil.MILLIS_IN_SECOND
      time += date.getUTCMilliseconds()
      return time
    },
    atomFromJsDate: function(date) {
      return new KdbTimeAtom(KdbTimeUtil.Time.millisFromJsDate(date))
    },
    toJsDate: function(millis) {
      return new Date(millis)
    },
    toString: function(time, suffix) {
      const nil = MgKdb.int_to_0N_str(time, KdbConstants.i32, suffix ? 't' : '')
      if (nil) return nil
      const hours = Math.floor(time / KdbTimeUtil.MILLIS_IN_HOUR)
      time -= hours * KdbTimeUtil.MILLIS_IN_HOUR
      const mins = Math.floor(time / KdbTimeUtil.MILLIS_IN_MINUTE)
      time -= mins * KdbTimeUtil.MILLIS_IN_MINUTE
      const secs = Math.floor(time / KdbTimeUtil.MILLIS_IN_SECOND)
      time -= secs * KdbTimeUtil.MILLIS_IN_SECOND
      const F2 = MgFormat.NUM_FMT_2D
      const F3 = MgFormat.NUM_FMT_3D
      return `${F2.format(hours)}:${F2.format(mins)}:${F2.format(secs)}.${F3.format(time)}`
    }
  }
}


class KdbType {
  constructor(typ, val) {      const days = 
    this.typ = typ
  }
}

class KdbAtom extends KdbType {
  constructor(typ, val) {
    super(typ)
    this.val = val
  }
}

export class KdbBoolAtom extends KdbAtom {
  constructor(val) {
    super(-1, !!val)
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.val !== 0
  }

  toString = () => this.val ? '1b' : '0b'
}

class KdbGuidAtom extends KdbAtom {
  constructor(val) {
    super(-2, MgKdb.chk_guid(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.val
  }

  toString = () => {
    return "'nyi"
  }
}

export class KdbByteAtom extends KdbAtom {

  static toHexDigits = function(val) {
    const hex = val.toString(16)
    return hex.length === 2 ? hex : `0${hex}`
  }

  constructor(val) {
    super(-4, MgKdb.chk_is_int8(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.val
  }

  toString = () => {
    return `0x${KdbByteAtom.toHexDigits(this.val)}`
  }
}

export class KdbShortAtom extends KdbAtom {
  constructor(val) {
    super(-5, MgKdb.chk_is_int16(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.val
  }

  toString = () => {
    const nil = MgKdb.int_to_0N_str(this.val, KdbConstants.i16, 'h')
    return null !== nil ? nil : `${this.val}h`
  }
}

export class KdbIntAtom extends KdbAtom {
  constructor(val) {
    super(-6, MgKdb.chk_is_int32(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.val
  }

  toString = () => {
    const nil = MgKdb.int_to_0N_str(this.val, KdbConstants.i32, 'i')
    return null !== nil ? nil : `${this.val}i`
  }
}

export class KdbLongAtom extends KdbAtom {
  constructor(val) {
    super(-7, MgKdb.chk_is_bigint(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    // BigInt(Number.MAX_SAFE_INTEGER) ?
    return Number(this.val)
  }

  toString = () => {
    const nil = MgKdb.int_to_0N_str(this.val, KdbConstants.i64, '')
    return nil ? nil : `${this.val.toString()}j`
  }
}

export class KdbRealAtom extends KdbAtom {
  constructor(val) {
    super(-8, MgKdb.chk_is_float(val)) 
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.val
  }

  toString = () => {
    const nil = MgKdb.fp_to_0N_str(this.val, 'e')
    return null !== nil ? nil : `${this.val}e`
  }
}

export class KdbFloatAtom extends KdbAtom {
  constructor(val) {
    super(-9, val) 
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.val
  }

  toString = () => {
    const nil = MgKdb.fp_to_0N_str(this.val, 'f')
    return null !== nil ? nil : `${this.val}f`
  }
}

export class KdbCharAtom extends KdbAtom {
  constructor(val) {
    super(-10, String.fromCharCode(MgKdb.chk_is_int8(val)))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.val
  }

  toString = () => `"${String.fromCharCode(this.val)}"`
}

export class KdbSymbolAtom extends KdbAtom {
  constructor(val) {
    super(-11, MgKdb.chk_is_string(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.val
  }

  toString = () => {
    if (-1 !== this.val.indexOf(' ')) {
      return `(\`$"${this.val}")`
    }
    return '`' + this.val
  }
}

export class KdbTimestampAtom extends KdbAtom {
  constructor(val) {
    super(-12, MgKdb.chk_is_bigint(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return KdbTimeUtil.Timestamp.toJsDate(this.val)
  }

  toString = () => KdbTimeUtil.Timestamp.toString(this.val, true)
}

export class KdbMonthAtom extends KdbAtom {
  constructor(val) {
    super(-13, MgKdb.chk_is_int32(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return KdbTimeUtil.Month.toJsDate(this.val)
  }

  toString = () => KdbTimeUtil.Month.toString(this.val, true)
}

export class KdbDateAtom extends KdbAtom {

  constructor(val) {
    super(-14, MgKdb.chk_is_int32(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return KdbTimeUtil.Date.toJsDate(this.val)
  }

  toString = () => KdbTimeUtil.Date.toString(this.val, true)
}

export class KdbDateTimeAtom extends KdbAtom {
  constructor(val) {
    super(-15, MgKdb.chk_is_float(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return new Date(-1)
  }

  toString = () => "'nyi"
}

export class KdbTimespanAtom extends KdbAtom {
  constructor(val) {
    super(-16, MgKdb.chk_is_bigint(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return KdbTimeUtil.Timespan.toJsDate(this.val)
  }

  toString = () => KdbTimeUtil.Timespan.toString(this.val, true)
}

export class KdbMinuteAtom extends KdbAtom {
  constructor(val) {
    super(-17, MgKdb.chk_is_int32(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return KdbTimeUtil.Minute.toJsDate(this.val)
  }

  toString = () => KdbTimeUtil.Minute.toString(this.val, true)
}

export class KdbSecondAtom extends KdbAtom {
  constructor(val) {
    super(-18, MgKdb.chk_is_int32(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return KdbTimeUtil.Second.toJsDate(this.val)
  }

  toString = () => KdbTimeUtil.Second.toString(this.val, true)
}

export class KdbTimeAtom extends KdbAtom {
  constructor(val) {
    super(-19, MgKdb.chk_is_int32(val))
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return KdbTimeUtil.Time.toJsDate(this.val)
  }

  toString = () => KdbTimeUtil.Time.toString(this.val, true)
}

export class KdbList extends KdbType {
  constructor(ary, att) {
    super(0)
    this.ary = ary
    this.att = att ? att : 0
  }

  count = () => this.ary.length

  getObjectAt = idx => this.ary[idx]

  getJsObjectAt = (cfg, idx) => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, idx, this) 
    return this.ary[idx].getJsObject(cfg)
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(val => val.getJsObject(cfg))
  }
  

  toString = () => {
    return '(' + this.ary.map(elm => elm.toString()).join(';') + ')'
  }
}

class KdbVector extends KdbType {
  constructor(typ, ary, att) {
    super(typ)
    this.ary = ary
    this.att = att
  }
  count = () => this.ary.length
}

export class KdbBoolVector extends KdbVector {
  constructor(ary, att) {
    super(1, ary, att)
  }

  getObjectAt = idx => new KdbBoolAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return this.ary[idx] !== 0
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(val => val !== 0)
  }
  
  toString = () => {
    if (this.ary.length === 0) return '(1h$())'
    if (this.ary.length === 1) return `(enlist ${this.ary[0]}b)`
    return this.ary.join('') + 'b'
  }
}

export class KdbByteVector extends KdbVector {
  constructor(ary, att) {
    super(4, ary, att)
  }

  getObjectAt = idx => new KdbByteAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return this.ary[idx]
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.slice()
  }
  toString = () => {
    if (this.ary.length === 0) return '(4h$())'
    if (this.ary.length === 1) return `(enlist 0x${KdbByteAtom.toHexDigits(this.ary[0])})`
    const elm = []
    for (let i = 0 ; i < this.ary.length ; i++) {
      elm.push(KdbByteAtom.toHexDigits(this.ary[i]))
    }
    return `0x${elm.join('')}`
  }
}

export class KdbShortVector extends KdbVector {
  constructor(ary, att) {
    super(5, ary, att)
  }

  getObjectAt = idx => new KdbShortAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return this.ary[idx]
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.slice()
  }

  toString = () => {
    if (this.ary.length === 0) return '(5h$())'
    if (this.ary.length === 1) return `(enlist ${this.ary[0]}h)`
    const elm = new Array(this.ary.length)
    for (let i = 0 ; i < this.ary.length ; i++) {
      const nil = MgKdb.int_to_0N_str(this.ary[i], KdbConstants.i16, '')
      elm[i] = null !== nil ? nil : this.ary[i]
    }
    return elm.join(' ') + 'h'
  }
}

export class KdbIntVector extends KdbVector {
  constructor(ary, att) {
    super(6, ary, att)
  }

  getObjectAt = idx => new KdbIntAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return this.ary[idx]
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.slice()
  }

  toString = () => {
    if (this.ary.length === 0) return '(6h$())'
    if (this.ary.length === 1) return `(enlist ${this.ary[0]}i)`
    const elm = new Array(this.ary.length)
    for (let i = 0 ; i < this.ary.length ; i++) {
      const nil = MgKdb.int_to_0N_str(this.ary[i], KdbConstants.i32, '')
      elm[i] = null !== nil ? nil : this.ary[i]
    }
    return elm.join(' ') + 'i'
  }
}

export class KdbLongVector extends KdbVector {
  constructor(ary, att) {
    super(7, ary, att) // TODO check tpe is BigInt64Array
  }

  getObjectAt = idx => new KdbLongAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    // BigInt(Number.MAX_SAFE_INTEGER) ? 
    return Number(this.ary[idx])
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(val => Number(val))
  }

  toString = () => {
    if (this.ary.length === 0) return '(7h$())'
    if (this.ary.length === 1) return `(enlist ${this.ary[0]}j)`
    const elm = new Array(this.ary.length)
    for (let i = 0 ; i < this.ary.length ; i++) {
      const nil = MgKdb.int_to_0N_str(this.ary[i], KdbConstants.i64, '')
      elm[i] = null !== nil ? nil : this.ary[i]
    }
    return elm.join(' ') + 'j'
  }
}

export class KdbRealVector extends KdbVector {
  constructor(ary, att) {
    super(8, ary, att)
  }

  getObjectAt = idx => new KdbRealAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return this.ary[idx]
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.slice()
  }

  toString = () => {
    if (this.ary.length === 0) return '(8h$())'
    if (this.ary.length === 1) return `(enlist ${this.ary[0]}e)`
    const elm = new Array(this.ary.length)
    for (let i = 0 ; i < this.ary.length ; i++) {
      const nil = MgKdb.fp_to_0N_str(this.ary[i], '')
      elm[i] = null !== nil ? nil : this.ary[i]
    }
    return elm.join(' ') + 'e'
  }
}

export class KdbFloatVector extends KdbVector {
  constructor(ary, att) {
    super(9, ary, att)
  }

  getObjectAt = idx => new KdbFloatAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return this.ary[idx]
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.slice()
  }

  toString = () => {
    if (this.ary.length === 0) return '(9h$())'
    if (this.ary.length === 1) return `(enlist ${this.ary[0]}f)`
    const elm = new Array(this.ary.length)
    for (let i = 0 ; i < this.ary.length ; i++) {
      const nil = MgKdb.fp_to_0N_str(this.ary[i], '')
      elm[i] = null !== nil ? nil : this.ary[i]
    }
    return elm.join(' ') + 'f'
  }
}

export class KdbCharVector extends KdbVector {
  static fromString = function(str) {
    str = CdotJS.u8u16(str)
    const len = str.length
    const ary = new Int8Array(len)
    for (let i = 0 ; i < len ; i++) {
      ary[i] = str[i]
    }
    return new KdbCharVector(ary)
  }

  constructor(ary, att) {
    super(10, ary, att)
    this.string = null
  }

  getObjectAt = idx => new KdbCharAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return this.asString()
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.asString()
  }

  asString = () => {
    if (this.string === null) {
      this.string = CdotJS.u16u8(this.ary)
    }
    return this.string
  }
  toString = () => `"${this.asString()}"`
}

export class KdbSymbolVector extends KdbVector {
  constructor(ary, att) {
    super(11, ary, att)
  }

  getObjectAt = idx => new KdbSymbolAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return this.ary[idx]
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.slice()
  }

  findIndex = sym => {
    return this.ary.findIndex(val => val === sym)
  }

  toString = () => {
    if (this.ary.length === 0) return '(`$())'
    if (this.ary.length === 1) return `(enlist \`$"${this.ary[0]}")`
    let any = false
    for (let i = 0 ; !any && i < this.ary.length ; i++) {
      any |= this.ary[i].indexOf(' ') !== -1
    }
    if (any) {
      return '(`$("' + this.ary.join('";"') + '"))'
    }
    return '`' + this.ary.join('`')
  }
}

export class KdbTimestampVector extends KdbVector {
  constructor(ary, att) {
    super(12, ary, att)
  }

  getObjectAt = idx => new KdbTimestampAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return KdbTimeUtil.Timestamp.toJsDate(this.ary[idx])
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(KdbTimeUtil.Timestamp.toJsDate)
  }

  toString = () => {
    if (this.ary.length === 0) return '(12h$())'
    if (this.ary.length === 1) return `(enlist ${KdbTimeUtil.Timestamp.toString(this.ary[0], true)})`
    const elm = new Array(this.ary.length)
    let all = true
    for (let i = 0 ; i < this.ary.length ; i++) {
      const sfx = all && i == this.ary.length - 1
      elm[i] = KdbTimeUtil.Timestamp.toString(this.ary[i], sfx)
      all &= elm[i].length <= 3
    }
    return elm.join(' ')
  }
}

export class KdbMonthVector extends KdbVector {
  constructor(ary, att) {
    super(13, ary, att)
  }

  getObjectAt = idx => new KdbMonthAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return KdbTimeUtil.Month.toJsDate(this.ary[idx])
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(KdbTimeUtil.Month.toJsDate)
  }

  toString = () => {
    if (this.ary.length === 0) return '(13h$())'
    if (this.ary.length === 1) return `(enlist ${KdbTimeUtil.Month.toString(this.ary[0], true)})`
    const elm = new Array(this.ary.length)
    for (let i = 0 ; i < this.ary.length ; i++) {
      const sfx = i == this.ary.length - 1
      elm[i] = KdbTimeUtil.Month.toString(this.ary[i], sfx)
    }
    return elm.join(' ')
  }
}

export class KdbDateVector extends KdbVector {
  constructor(ary, att) {
    super(14, ary, att)
  }

  getObjectAt = idx => new KdbDateAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return KdbTimeUtil.Date.toJsDate(this.ary[idx])
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(KdbTimeUtil.Date.toJsDate)
  }

  toString = () => {
    if (this.ary.length === 0) return '(14h$())'
    if (this.ary.length === 1) return `(enlist ${KdbTimeUtil.Date.toString(this.ary[0], true)})`
    const elm = new Array(this.ary.length)
    let all = true
    for (let i = 0 ; i < elm.length ; i++) {
      const sfx = all && i == this.ary.length - 1
      elm[i] = KdbTimeUtil.Date.toString(this.ary[i], sfx)
      all &= elm[i].length <= 3
    }
    return elm.join(' ')
  }
}

class KdbDateTimeVector extends KdbVector {
  constructor(ary, att) {
    super(15, ary, att)
  }

  getObjectAt = idx => new KdbDateTimeAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return new Date(-1)
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(val => new Date(-1))
  }

  toString = () => {
    if (this.ary.length === 0) return '(15h$())'
    if (this.ary.length === 1) return "(enlist 'nyi)"
    const elm = new Array(this.ary.length)
    for (let i = 0 ; i < elm.length ; i++) {
      elm[i] = "'nyi"
    }
    return elm.join(' ')
  }

}

export class KdbTimespanVector extends KdbVector {
  constructor(ary, att) {
    super(16, ary, att)
  }

  getObjectAt = idx => new KdbTimespanAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return KdbTimeUtil.Timespan.toJsDate(this.ary[idx])
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(KdbTimeUtil.Timespan.toJsDate)
  }

  toString = () => {
    if (this.ary.length === 0) return '(16h$())'
    if (this.ary.length === 1) return `(enlist ${KdbTimeUtil.Timespan.toString(this.ary[0], true)})`
    const elm = new Array(this.ary.length)
    let all = true
    for (let i = 0 ; i < elm.length ; i++) {
      const sfx = all && i == this.ary.length - 1
      elm[i] = KdbTimeUtil.Timespan.toString(this.ary[i], sfx)
      all &= elm[i].length <= 3
    }
    return elm.join(' ')
  }
}

export class KdbMinuteVector extends KdbVector {
  constructor(ary, att) {
    super(17, ary, att)
  }

  getObjectAt = idx => new KdbMinuteAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return KdbTimeUtil.Minute.toJsDate(this.ary[idx])
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(KdbTimeUtil.Minute.toJsDate)
  }

  toString = () => {
    if (this.ary.length === 0) return '(17h$())'
    if (this.ary.length === 1) return `(enlist ${KdbTimeUtil.Minute.toString(this.ary[0], true)})`
    const elm = new Array(this.ary.length)
    let all = true
    for (let i = 0 ; i < elm.length ; i++) {
      const sfx = all && i == this.ary.length - 1
      elm[i] = KdbTimeUtil.Minute.toString(this.ary[i], sfx)
      all &= elm[i].length <= 3
    }
    return elm.join(' ')
  }
}

export class KdbSecondVector extends KdbVector {
  constructor(ary, att) {
    super(18, ary, att)
  }

  getObjectAt = idx => new KdbSecondAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return KdbTimeUtil.Second.toJsDate(this.ary[idx])
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(KdbTimeUtil.Second.toJsDate)
  }

  toString = () => {
    if (this.ary.length === 0) return '(18h$())'
    if (this.ary.length === 1) return `(enlist ${KdbTimeUtil.Second.toString(this.ary[0], true)})`
    const elm = new Array(this.ary.length)
    let all = true
    for (let i = 0 ; i < elm.length ; i++) {
      const sfx = all && i == this.ary.length - 1
      elm[i] = KdbTimeUtil.Second.toString(this.ary[i], sfx)
      all &= elm[i].length <= 3
    }
    return elm.join(' ')
  }
}

export class KdbTimeVector extends KdbVector {
  constructor(ary, att) {
    super(19, ary, att)
  }

  getObjectAt = idx => new KdbTimeAtom(this.ary[idx])

  getJsObjectAt = (cfg, idx) => {
    if (cfg[-this.typ]) return cfg[-this.typ](cfg, this.ary[idx]) 
    return KdbTimeUtil.Time.toJsDate(this.ary[idx])
  }

  getJsObject = cfg => {
    if (cfg[this.typ]) return cfg[this.typ](cfg, this)
    return this.ary.map(KdbTimeUtil.Time.toJsDate)
  }

  toString = () => {
    if (this.ary.length === 0) return '(19h$())'
    if (this.ary.length === 1) return `(enlist ${KdbTimeUtil.Time.toString(this.ary[0], true)})`
    const elm = new Array(this.ary.length)
    let all = true
    for (let i = 0 ; i < elm.length ; i++) {
      const sfx = all && i == this.ary.length - 1
      elm[i] = KdbTimeUtil.Time.toString(this.ary[i], sfx)
      all &= elm[i].length <= 3
    }
    return elm.join(' ')
  }
}

export class KdbTable extends KdbType {
  constructor(cols, vals, att) {
    super(98)
    this.cols = cols
    this.vals = vals
    this.att = att
  }

  count = () => this.vals.ary[0].count

  getObjectAt = idx => {
    const vals = this.vals.ary.map(vec => vec.getObjectAt(idx))
    return new KdbDict(this.cols, new KdbList(0, vals))
  }

  getJsObjectAt = (cfg, idx) => {
    if (cfg[98]) return cfg[98](cfg, idx, this.getObjectAt(idx))
    const cls = this.cols.ary
    const vls = this.vals.ary
    return Object.fromEntries(cls.map((col, c) => [col, vls[c].getJsObjectAt(cfg, idx)]))
  }

  getJsObject = cfg => {
    if (cfg[99]) return cfg[99](cfg, this)
    const cls = this.cols.ary
    const vls = this.vals.ary
    const ary = new Array(vls[0].count())
    for (let i = 0 ; i < ary.length ; i++) {
      ary[i] = this.getJsObjectAt(cfg, i)
    }
    return ary
  }

  findColIdx = col => {
    return this.cols.findIndex(col)
  }

  toString = () => `(flip${this.cols}!${this.vals})`
}

export class KdbDict extends KdbType {
  constructor(keys, vals) {
    super(99)
    this.keys = keys
    this.vals = vals
  }
  toString = () => `(${this.keys}!${this.vals})`
}

export class KdbFunction extends KdbType {
  constructor(txt) {
    super(100)
    this.txt = txt
  }
  toString = () => this.txt
}

export class KdbUnary extends KdbType {
  static VALS = [
      '::', 'flip', 'neg', 'first', 'reciprocal', 'where', 'reverse', 'null', 'group', 'iasc', 'hclose',
    'string', ',:', 'count', 'floor', 'not', 'inv', 'distinct', 'type', 'get', 'read0', 'read1', '2::',
    'avg', 'last', 'sum', 'prd', 'min', 'max', 'exit', 'getenv', 'abs', 'sqrt', 'log', 'exp', 'sin',
    'asin', 'cos', 'acos', 'tan', 'atan', 'enlist', 'var'
  ]
  static valueFromText = function(txt) {
    const idx =  KdbUnary.findIndex(txt)
    if (-1 === idx) {
      throw new SyntaxError("Lookup failed: unknown unary '" + txt + "'")
    }
    return idx
  }
  constructor(val) {
    super(101)
    if (!Number.isInteger(val) || val < -1 || val > KdbUnary.VALS.length) {
      throw new TypeError("'type")
    }
    this.val = val
  }
  toString = () => {
    if (this.val === 0xFF) {
      return '::'
    }
    return KdbUnary.VALS[this.val]
  }
}

export class KdbBinary extends KdbType {
  static VALS = [
    '::', '+', '-', '*', '%', '&', '|', '^', '=', '<', '>', '$', ',', '#', 
    '_', '~', '!', '?', '@', '.', '0:', '1:', '2:', 'in', 'within', 'like', 
    'bin', 'ss', 'insert', 'wsum', 'wavg', 'div', 'xexp', 'setenv', 'binr',
    'cov', 'cor', 
  ]
  static valueFromText = function(txt) {
    const idx =  KdbBinary.findIndex(txt)
    if (-1 === idx) {
      throw new SyntaxError("Lookup failed: unknown binary '" + txt + "'")
    }
    return idx
  }
  constructor(val) {
    super(101)
    if (!Number.isInteger(val) || val < 0 || val > KdbBinary.VALS.length) {
      throw new TypeError("'type")
    }
    this.val = val
  }
  toString = () => {
    return KdbBinary.VALS[this.val]
  }
}

export class KdbTernary extends KdbType {
  static VALS = [
    '\'', '/', '\\', '\':', '/:', '\\:', '*/'
  ]
  static valueFromText = function(txt) {
    const idx =  KdbTernary.findIndex(txt)
    if (-1 === idx) {
      throw new SyntaxError("Lookup failed: unknown binary '" + txt + "'")
    }
    return idx
  }

  constructor(val) {
    super(103)
    if (!Number.isInteger(val) || val < 0 || val > KdbTernary.VALS.length) {
      throw new RangeError('oob')
    }
    this.val = val
  }
  toString = () => {
    if (this.val < 0 || this.val > KdbTernary.VALS.length) {
      throw new RangeError("'oob")
    }
    return KdbTernary.VALS[this.val]
  }
}

export class KdbProjection extends KdbType {
  constructor(fun, rgs) {
    super(104)
    if (typeof(fun) === 'string') {
      fun = new KdbFunction(fun)
    }
    if (typeof(fun) !== 'object' || fun.constructor !== KdbFunction) {
      throw new TypeError("'bad_function")
    }
    if (typeof(rgs) !== 'object' || rgs.constructor !== Array) {
      throw new TypeError("'bad.args")
    }
    this.fun = fun
    this.rgs = rgs
  }
  toString = () => {
    return this.fun + '[' + this.rgs.map(arg => arg.toString()).join(';') + ']'
  }
}


class _KdbReader {
  constructor(aryBuf) {
    this.u8y = new Uint8Array(aryBuf)
    this.i8y = new Int8Array(aryBuf)
    this.buf = new Uint8Array(8)
    this.i16 = new Int16Array(this.buf.buffer)
    this.i32 = new Int32Array(this.buf.buffer)
    this.i64 = new BigInt64Array(this.buf.buffer)
    this.f32 = new Float32Array(this.buf.buffer)
    this.f64 = new Float64Array(this.buf.buffer)
    if (this.i8y[0] !== 1) {
      throw new Error("Cannot decode little-endian messages")
    }
    this.typ = this.i8y[1]
    this.pos = 8
  }
  copyN = len => {
    for(let i = 0 ; i < len ; i++) {
      this.buf[i] = this.u8y[this.pos++]
    }
  }
  readI16 = () => {
    this.copyN(2)
    return this.i16[0]
  }
  readI32 = () => {
    this.copyN(4)
    return this.i32[0]
  }
  readI64 = () => {
    this.copyN(8)
    return this.i64[0]
  }
  readF32 = () => {
    this.copyN(4)
    return this.f32[0]
  }
  readF64 = () => {
    this.copyN(8)
    return this.f64[0]
  }
  readSym = () => {
    const off = this.pos
    while (0 !== this.i8y[this.pos++]) {}
    const ary = this.i8y.slice(off, this.pos - 1)
    return CdotJS.u16u8(ary)
  }
  readI32Ary = () => {
    const ary = new Int32Array(this.readI32())
    for (let i = 0 ; i < ary.length ; i++) {
      ary[i] = this.readI32()
    }
    return ary
  }
  readI64Ary = () => {
    const ary = new BigInt64Array(this.readI32())
    for (let i = 0 ; i < ary.length ; i++) {
      ary[i] = this.readI64()
    }
    return ary
  }
  readBoolAtom = () => new KdbBoolAtom(0 !== this.i8y[this.pos++])
  readByteAtom = () => new KdbByteAtom(this.u8y[this.pos++])
  readShortAtom = () => {
    let val = this.readI16()
    // TODO NaN etc
    return new KdbShortAtom(val)
  }
  readIntAtom = () => {
    let val = this.readI32()
    // TODO NaN etc
    return new KdbIntAtom(val)
  }
  readLongAtom = () => {
    let val = this.readI64()
    // TODO NaN ...
    return new KdbLongAtom(val)
  }
  readRealAtom = () => {
    let val = this.readF32()
    // NaN ...
    return new KdbRealAtom(val)
  }
  readFloatAtom = () => {
    let val = this.readF64()
    // NaN ...
    return new KdbFloatAtom(val)
  }
  readCharAtom = () => {
    return new KdbCharAtom(this.i8y[this.pos++])
  }
  readSymbolAtom = () => {
    return new KdbSymbolAtom(this.readSym())
  }
  readTimestampAtom = () => {
    return new KdbTimestampAtom(this.readI64())
  }
  readMonthAtom = () => {
    return new KdbMonthAtom(this.readI32())
  }
  readDateAtom = () => {
    return new KdbDateAtom(this.readI32())
  }
  readDateTimeAtom = () => {
    return new KdbDateTimeAtom(this.readF64())
  }
  readTimespanAtom = () => {
    return new KdbTimespanAtom(this.readI64())
  }
  readMinuteAtom = () => {
    return new KdbMinuteAtom(this.readI32())
  }
  readSecondAtom = () => {
    return new KdbSecondAtom(this.readI32())
  }
  readTimeAtom = () => {
    return new KdbTimeAtom(this.readI32())
  }

  readList = () => {
    const att = this.i8y[this.pos++]
    const len = this.readI32()
    const ary = new Array(len)
    for (let i = 0 ; i < len ; i++) {
      ary[i] = this.read()
    }
    return new KdbList(ary, att)
  }
  readBoolVector = () => {
    const att = this.i8y[this.pos++]
    const len = this.readI32()
    const off = this.pos
    this.pos += len
    return new KdbBoolVector(this.i8y.slice(off, off + len), att)
  }
  readGuidVector = () => {
    throw new Error("'nyi")
  }
  readByteVector = () => {
    const att = this.i8y[this.pos++]
    const len = this.readI32()
    const off = this.pos
    this.pos += len
    return new KdbByteVector(this.u8y.slice(off, off + len), att)
  }
  readShortVector = () => {
    const att = this.i8y[this.pos++]
    const ary = new Int16Array(this.readI32())
    for (let i = 0 ; i < ary.length ; i++) {
      ary[i] = this.readI16()
    }
    return new KdbShortVector(ary, att)
  }
  readIntVector = () => {
    const att = this.i8y[this.pos++]
    return new KdbIntVector(this.readI32Ary(), att)
  }
  readLongVector = () => {
    const att = this.i8y[this.pos++]
    return new KdbLongVector(this.readI64Ary(), att)
  }
  readRealVector = () => {
    const att = this.i8y[this.pos++]
    const ary = new Float32Array(this.readI32())
    for (let i = 0 ; i < ary.length ; i++) {
      ary[i] = this.readF32()
    }
    return new KdbRealVector(ary, att)
  }
  readFloatVector = () => {
    const att = this.i8y[this.pos++]
    const ary = new Float64Array(this.readI32())
    for (let i = 0 ; i < ary.length ; i++) {
      ary[i] = this.readF64()
    }
    return new KdbFloatVector(ary, att)
  }
  readCharVector = () => {
    const att = this.i8y[this.pos++]
    const len = this.readI32()
    const off = this.pos
    this.pos += len
    return new KdbCharVector(this.i8y.slice(off, off + len), att)
  }
  readSymbolVector = () => {
    const att = this.i8y[this.pos++]
    const len = this.readI32()
    const ary = new Array(len)
    for (let i = 0 ; i < len ; i++) {
      ary[i] = this.readSym()
    }
    return new KdbSymbolVector(ary, att)
  }
  readTimestampVector = () => {
    const att = this.i8y[this.pos++]
    return new KdbTimestampVector(this.readI64Ary(), att)
  }
  readMonthVector = () => {
    const att = this.i8y[this.pos++]
    return new KdbMonthVector(this.readI32Ary(), att)
  }
  readDateVector = () => {
    const att = this.i8y[this.pos++]
    return new KdbDateVector(this.readI32Ary(), att)
  }
  readDateTimeVector = () => {
    const att = this.i8y[this.pos++]
    const ary = new Float64Array(this.readI32())
    for (let i = 0 ; i < ary.length ; i++) {
      ary[i] = this.readF64()
    }
    throw new KdbDateTimeVector(ary, att)
  }
  readTimespanVector = () => {
    const att = this.i8y[this.pos++]
    return new KdbTimespanVector(this.readI64Ary(), att)
  }
  readMinuteVector = () => {
    const att = this.i8y[this.pos++]
    return new KdbMinuteVector(this.readI32Ary(), att)
  }
  readSecondVector = () => {
    const att = this.i8y[this.pos++]
    return new KdbSecondVector(this.readI32Ary(), att)
  }
  readTimeVector = () => {
    const att = this.i8y[this.pos++]
    return new KdbTimeVector(this.readI32Ary(), att)
  }
  readTable = () => {
    const att = this.i8y[this.pos++]
    const dct = this.i8y[this.pos++]
    const cols = this.read()
    const vals = this.read()
    return new KdbTable(cols, vals, att)
  }
  readDict = () => {
    const keys = this.read()
    const vals = this.read()
    return new KdbDict(keys, vals)
  }
  readFunction = () => {
    this.pos++ // function attribute byte
    this.pos++ // char-vec type 0x0a
    this.pos++ // char-vec attribute
    const len = this.readI32()
    const off = this.pos
    this.pos += len
    return new KdbFunction(CdotJS.u16u8(this.i8y.slice(off, off + len)))
  }
  readUnary = () => {
    return new KdbUnary(this.i8y[this.pos++])
  }
  readBinary = () => {
    return new KdbBinary(this.i8y[this.pos++])
  }
  readTernary = () => {
    return new KdbTernary(this.i8y[this.pos++])
  }
  readProjection = () => {
    const len = this.readI32() - 1
    const fun = this.read()
    const ary = new Array(len)
    for (let i = 0 ; i < len ; i++) {
      ary[i] = this.read()
    }
    return new KdbProjection(fun, ary)
  }
  read = () => {
    let typ = this.i8y[this.pos++]
    switch (typ) {
      case -19: return this.readTimeAtom()
      case -18: return this.readSecondAtom()
      case -17: return this.readMinuteAtom()
      case -16: return this.readTimespanAtom()
      case -15: return this.readDateTimeAtom()
      case -14: return this.readDateAtom()
      case -13: return this.readMonthAtom()
      case -12: return this.readTimestampAtom()
      case -11: return this.readSymbolAtom()
      case -10: return this.readCharAtom()
      case  -9: return this.readFloatAtom()
      case  -8: return this.readRealAtom()
      case  -7: return this.readLongAtom()
      case  -6: return this.readIntAtom()
      case  -5: return this.readShortAtom()
      case  -4: return this.readByteAtom()
      case  -2: return this.readGuidAtom()
      case  -1: return this.readBoolAtom()
      case   0: return this.readList()
      case   1: return this.readBoolVector()
      case   2: return this.readGuidVector()
      case   4: return this.readByteVector()
      case   5: return this.readShortVector()
      case   6: return this.readIntVector()
      case   7: return this.readLongVector()
      case   8: return this.readRealVector()
      case   9: return this.readFloatVector()
      case  10: return this.readCharVector()
      case  11: return this.readSymbolVector()
      case  12: return this.readTimestampVector()
      case  13: return this.readMonthVector()
      case  14: return this.readDateVector()
      case  15: return this.readDateTimeVector()
      case  16: return this.readTimespanVector()
      case  17: return this.readMinuteVector()
      case  18: return this.readSecondVector()
      case  19: return this.readTimeVector()
      case  98: return this.readTable()
      case  99: return this.readDict()
      case 100: return this.readFunction()
      case 101: return this.readUnary()
      case 102: return this.readBinary()
      case 103: return this.readTernary()
      case 104: return this.readProjection()
      default:
        throw new Error("'nyi: " + typ)
    }
  }
}

class _KdbWriter {
  constructor(msgTyp, msg) {
    this.msgTyp = msgTyp
    this.msg = msg
    this.psz = this.writeSz(msg)
    this.msz = 8 + this.psz
    this.dst = new ArrayBuffer(this.msz)
    this.buf = new Uint8Array(this.dst)
    this.tmp = new Uint8Array(8)
    this.i32 = new Int32Array(this.tmp.buffer)
    this.i64 = new BigInt64Array(this.tmp.buffer)
    this.f32 = new Float32Array(this.tmp.buffer)
    this.f64 = new Float64Array(this.tmp.buffer)
  }
  writeI8  = val => this.buf[this.pos++] = val
  writeI16 = val => { this.i32[0] = val; for (let i = 0 ; i < 2 ; i++) this.buf[this.pos++] = this.tmp[i] }
  writeI32 = val => { this.i32[0] = val; for (let i = 0 ; i < 4 ; i++) this.buf[this.pos++] = this.tmp[i] }
  writeI64 = val => { this.i64[0] = val; for (let i = 0 ; i < 8 ; i++) this.buf[this.pos++] = this.tmp[i] }
  writeF32 = val => { this.f32[0] = val; for (let i = 0 ; i < 4 ; i++) this.buf[this.pos++] = this.tmp[i] }
  writeF64 = val => { this.f32[0] = val; for (let i = 0 ; i < 8 ; i++) this.buf[this.pos++] = this.tmp[i] }
  writeSym = val => {
    const sym = CdotJS.u8u16(val)
    for (let i = 0 ; i < sym.length ; i++) {
      this.writeI8(sym[i])
    }
    this.writeI8(0)
  }
  writeVecHdr = vec => {
    this.writeI8(vec.typ)
    this.writeI8(vec.att)
    this.writeI32(vec.ary.length)
  }
  writeVec = (wfn, vec) => {
    this.writeVecHdr(vec)
    vec.ary.forEach(wfn)
  }
  writeSz = elm => {
    switch(elm.typ) {
      case -19:
      case -18:
      case -17:
      case -14:
      case -13:
      case  -8:
      case  -6:
        return 5
      case -16:
      case -15:
      case -12:
      case  -9:
      case  -7:
        return 9
      case  -5:
        return 3
      case -11:
        return 1 + CdotJS.u8u16(elm.val).length + 1
      case -10:
      case  -4:
      case  -1:
      case 101:
      case 102:
      case 103:
        return 2
      case   0:
        return elm.ary.reduce((sum, val) => sum + this.writeSz(val), 6)
      case   1:
      case   4:
      case  10:
        return 6 + elm.ary.length
      case  11:
        return elm.ary.reduce((sum, val) => sum + CdotJS.u8u16(val).length + 1, 6)
      case   5:
        return 6 + 2 * elm.ary.length
      case   7:
      case   9:
      case  12:
      case  15:
      case  16:
        return 6 + 8 * elm.ary.length
      case   6:
      case   8:
      case  13:
      case  14:
      case  17:
      case  18:
      case  19:
        return 6 + 4 * elm.ary.length
      case  98:
        return 3 + this.writeSz(elm.keys) + this.writeSz(elm.vals)
      case  99:
        return 1 + this.writeSz(elm.keys) + this.writeSz(elm.vals)
      case 100:
      case 104:
      default:
        throw new Error("'nyi: " + elm.typ)
    }
  }
  writeElm = elm => {
    switch (elm.typ) {
      case -19:
      case -18:
      case -17:
      case -14:
      case -13:
      case  -6:
        this.writeI8(elm.typ)
        return this.writeI32(elm.val)
      case -16:
      case -15:
      case -12:
      case  -7:
        this.writeI8(elm.typ)
        return this.writeI64(elm.val)
      case -15:
      case  -9:
        this.writeI8(elm.typ)
        return this.writeF64(elm.val)
      case  -8:
        this.writeI8(elm.typ)
        return this.writeF32(elm.val)
      case  -5:
        this.writeI8(elm.typ)
        return this.writeI16(elm.val)
      case -10:
      case  -4:
      case  -1:
      case 101:
      case 102:
      case 103:
        this.writeI8(elm.typ)
        return this.writeI8(elm.val)
      case -11: {
        this.writeI8(elm.typ)
        this.writeSym(elm.val)
        return
      }
      case   0:
        return this.writeVec(this.writeElm, elm)
      case   1:
      case   4:
      case  10:
        return this.writeVec(this.writeI8, elm)
      case   5:
        return this.writeVec(this.writeI16, elm)
      case   6:
      case  13:
      case  14:
      case  17:
      case  18:
      case  19:
        return this.writeVec(this.writeI32, elm)
      case   7:
      case  12:
      case  16:
        return this.writeVec(this.writeI64, elm)
      case   8:
        return this.writeVec(this.writeF32, elm)
      case   9:
      case  15:
        return this.writeVec(this.writeF64, elm)
      case  11: {
        this.writeVecHdr(elm)
        for (let i = 0 ; i < elm.ary.length ; i++) {
          this.writeSym(elm.ary[i])
        }
      }
      case  98: 
        this.writeI8(elm.typ)
        this.writeI8(elm.att)
        this.writeI8(99)
        this.writeElm(elm.keys)
        this.writeElm(elm.vals)
        return
      case  99: 
        this.writeI8(elm.typ)
        this.writeElm(elm.keys)
        this.writeElm(elm.vals)
        return
      case 100:
      case 104:
      default:
        throw new Error("'nyi : " + elm.typ)
    }
  }
  write = () => {
    this.pos = 0
    this.writeI8(1)
    this.writeI8(this.msgTyp)
    this.writeI8(0)
    this.writeI8(0)
    this.writeI32(this.msz)
    this.writeElm(this.msg)

    return this.buf
  }
}

MgKdb.deserialize = function(aryBuf) {
  const rdr = new _KdbReader(aryBuf)
  const msg = {
    typ: rdr.typ,
    msg: rdr.read()
  }
  return msg
}

MgKdb.serialize = function(msgTyp, msg) {
  const wtr = new _KdbWriter(msgTyp, msg)
  const buf = wtr.write()
  const len = buf.length
  const str = []
  // for (let i = 0 ; i < len ; i++) {
  //   str.push(KdbByteAtom.toHexDigits(buf[i]))
  // }
  // console.log("Serialized: 0x" + str.join(''))
  return buf
}

const KdbMsgTyp = {
  ASYNC: 0,
  SYNC: 1,
  RESPONSE: 2,
}

class _MgWs {
  constructor(url, cbk) {
    this.url = url
    this.ws = new WebSocket(url)
    this.ws.binaryType = 'arraybuffer'
    this.ws.onopen = this._wsOpened
    this.ws.onclose = this._wsClosed
    this.ws.onerror = this._wsError
    this.ws.onmessage = this._wsMsg
    this.isConnected = false
    this.listener = cbk
    this.queryId = 0
    this.queryTracker = new Map()
  }
  sendRequest = (msg, cbk) => {
    if (!this.isConnected) {
      throw new Error("Not connected")
    }
    if (typeof(cbk) !== 'function') {
      throw new TypeError("Callback must be a function")
    }
    const hdr = new KdbSymbolAtom('.web.request')
    const qid = new KdbIntAtom(this.queryId++)
    if (msg.constructor === Array) {
      msg = new KdbList(msg.toSpliced(1, 0, qid))
    }
    else if (msg.constructor === KdbList) {
      msg.ary = msg.ary.toSpliced(1, 0, qid)
    }
    const req = new KdbList([hdr, msg])
    const buf = MgKdb.serialize(KdbMsgTyp.SYNC, req)
    this.ws.send(buf)
    this.queryTracker.set(qid.val, cbk)
  }
  sendMessage = msg => {

  }
  close = () => {
    this.ws.close()
  }
  _wsOpened = evt => { // evt: generic Event
    this.isConnected = true
    console.debug("DEBUG: Connection opened to %s", this.url)
    if (typeof(this.listener.onConnected) === 'function') {
      this.listener.onConnected(this)
    }
  }
  _wsClosed = evt => { // evt ... reason, wasClean, code, 
    this.isConnected = false
    console.debug("DEBUG: Connection to %s closed", this.url)
    if (typeof(this.listener.onDisconnected) === 'function') {
      this.listener.onDisconnected(this)
    }
  }
  _wsError = evt => { // evt: generic Error event
    console.error("ERROR: WebSocket to %s signalled error: ", this.url, evt)
    if (typeof(this.listener.onError) === 'function') {
      this.listener.onError(this, evt)
    }
  }
  _wsMsg = evt => { // evt ... data, origin, 
    const obj = MgKdb.deserialize(evt.data)
    // console.log("Deserialized message: ", obj.msg.toString(), obj.msg)
    console.log("Deserialised message of type %d", obj.msg.typ)
    if (KdbMsgTyp.ASYNC === obj.typ) {
      // assume response like (`.web.response;qryId;isError;data)
      const msg = obj.msg
      if (msg.constructor === KdbList && msg.count() === 4 &&
          msg.ary[0].constructor === KdbSymbolAtom && msg.ary[0].val === '.web.response' &&
          msg.ary[1].constructor === KdbIntAtom && 
          msg.ary[2].constructor === KdbBoolAtom) {
        const [_fun, qid, isErr, data] = msg.ary
        if (this.queryTracker.has(qid.val)) {
          const cbk = this.queryTracker.get(qid.val)
          this.queryTracker.delete(qid.val)
          cbk(isErr, data)
        }
      }
      else {
        console.warn("WARN: dropped response: ", msg)
      }
    } // if-is-response
  }

}

MgKdb.Connection = _MgWs

const CdotJS = {}

// https://code.kx.com/platform/stream/dw_qr_ws_appendix/#cjs
// 2016.09.15 performance enhancement for temporal constructors and type identification
// 2016.03.18 char vectors and symbols now [de]serialize [from]to utf8
// 2014.03.18 Serialize date now adjusts for timezone.
// 2013.04.29 Dict decodes to map, except for keyed tables.
// 2013.02.13 Keyed tables were not being decoded correctly.
// 2012.06.20 Fix up browser compatibility. Strings starting with ` encode as symbol type.
// 2012.05.15 Provisional test release, subject to change
// for use with websockets and kdb+v3.0, (de)serializing kdb+ ipc formatted data within javascript within a browser.
// e.g. on kdb+ process, set .z.ws:{neg[.z.w] -8!value -9!x;}
// and then within javascript websocket.send(serialize("10+20"));
// ws.onmessage=function(e){var arrayBuffer=e.data;if(arrayBuffer){var v=deserialize(arrayBuffer);...
// note ws.binaryType = 'arraybuffer';


CdotJS.u8u16 = function (u16){
  var u8=[];
  for(var i=0;i<u16.length;i++){
    var c=u16.charCodeAt(i);
    if(c<0x80)u8.push(c);
    else if(c<0x800)u8.push(0xc0|(c>>6),0x80|(c&0x3f));
    else if(c<0xd800||c>=0xe000)u8.push(0xe0|(c>>12),0x80|((c>>6)&0x3f),0x80|(c&0x3f));
    else{
      c=0x10000+(((c&0x3ff)<<10)|(u16.charCodeAt(++i)&0x3ff));
      u8.push(0xf0|(c>>18),0x80|((c>>12)&0x3f),0x80|((c>>6)&0x3f),0x80|(c&0x3f));
    }
  }
  return u8;
}

CdotJS.u16u8 = function(u8){
  var u16="",c,c1,c2;
  for(var i=0;i<u8.length;i++)
    switch((c=u8[i])>>4){ 
      case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:u16+=String.fromCharCode(c);break;
      case 12:case 13:c1=u8[++i];u16+=String.fromCharCode(((c&0x1F)<<6)|(c1&0x3F));break;
      case 14:c1=u8[++i];c2=u8[++i];u16+=String.fromCharCode(((c&0x0F)<<12)|((c1&0x3F)<<6)|((c2&0x3F)<<0));break;
    }
  return u16;
}

CdotJS.deserialize = function(x){
  var a=x[0]
    ,pos=8
    ,j2p32=Math.pow(2,32)
    ,ub=new Uint8Array(x)
    ,sb=new Int8Array(x)
    ,bb=new Uint8Array(8)
    ,hb=new Int16Array(bb.buffer)
    ,ib=new Int32Array(bb.buffer)
    ,eb=new Float32Array(bb.buffer)
    ,fb=new Float64Array(bb.buffer);
  const msDay=86400000, QEpoch = msDay*10957;
  function rBool(){return rInt8()==1;}
  function rChar(){return String.fromCharCode(rInt8());}
  function rInt8(){return sb[pos++];}
  function rNUInt8(n){for(var i=0;i<n;i++)bb[i]=ub[pos++];}
  function rUInt8(){return ub[pos++];}
  function rGuid(){var x="0123456789abcdef",s="";for(var i=0;i<16;i++){var c=rUInt8();s+=i==4||i==6||i==8||i==10?"-":"";s+=x[c>>4];s+=x[c&15];}return s;}
  function rInt16(){rNUInt8(2);var h=hb[0];return h==-32768?NaN:h==-32767?-Infinity:h==32767?Infinity:h;}
  function rInt32(){rNUInt8(4);var i=ib[0];return i==-2147483648?NaN:i==-2147483647?-Infinity:i==2147483647?Infinity:i;}
  function rInt64(){rNUInt8(8);var x=ib[1],y=ib[0];return x*j2p32+(y>=0?y:j2p32+y);}// closest number to 64 bit int...
  function rFloat32(){rNUInt8(4);return eb[0];}
  function rFloat64(){rNUInt8(8);return fb[0];}
  function rSymbol(){var i=pos,c,s=[];while((c=rUInt8())!==0)s.push(c);return CdotJS.u16u8(s);};
  function rTimestamp(){return date(rInt64()/1000000);}
  function rMonth(){return new Date(Date.UTC(2000,rInt32(),1));}
  function date(n){return new Date(QEpoch+n);}
  function rDate(){return date(rInt32()*msDay);}
  function rDateTime(){return date(rFloat64()*msDay);}
  function rTimespan(){return date(rInt64()/1000000);}
  function rSecond(){return date(rInt32()/1000);}
  function rMinute(){return date(rInt32()*60000);}
  function rTime(){return date(rInt32());}
  function r(){
    var fns=[r,rBool,rGuid,null,rUInt8,rInt16,rInt32,rInt64,rFloat32,rFloat64,rChar,rSymbol,rTimestamp,rMonth,rDate,rDateTime,rTimespan,rMinute,rSecond,rTime];
    var i=0,n,t=rInt8();
    if(t<0&&t>-20)return fns[-t]();
    if(t>99){
      if(t==100){rSymbol();return r();}
      if(t<104)return rInt8()===0&&t==101?null:"func";
      if(t>105)r();
      else for(n=rInt32();i<n;i++)r();
      return"func";}
    if(99==t){
      var flip=98==ub[pos],x=r(),y=r(),o;
      if(!flip){
        o={};
        for(var i=0;i<x.length;i++)
          o[x[i]]=y[i];
      }else
        o=new Array(2),o[0]=x,o[1]=y;
      return o;
    }
    pos++;
    if(98==t){
 //    return r(); // better as array of dicts?
      rInt8(); // check type is 99 here
    // read the arrays and then flip them into an array of dicts
      var x=r(),y=r();
      var A=new Array(y[0].length);
      for(var j=0;j<y[0].length;j++){
        var o={};
        for(var i=0;i<x.length;i++)
          o[x[i]]=y[i][j];
        A[j]=o;}
      return A;}
    n=rInt32();
    if(10==t){var s=[];n+=pos;for(;pos<n;s.push(rUInt8()));return CdotJS.u16u8(s);}
    var A=new Array(n);
    var f=fns[t];
    for(i=0;i<n;i++)A[i]=f();
    return A;}
  return r();
}

CdotJS.serialize = function(x){
  var a=1,pos=0,ub,bb=new Uint8Array(8),ib=new Int32Array(bb.buffer),fb=new Float64Array(bb.buffer);
  function toType(obj) {var jsType=typeof obj;if(jsType!=='object'&&jsType!=='function') return jsType;
    if(!obj)return 'null';if(obj instanceof Array)return 'array';if(obj instanceof Date)return 'date';return 'object';}
  function getKeys(x){return Object.keys(x);}
  function getVals(x){var v=[];for(var o in x)v.push(x[o]);return v;}
  function calcN(x,dt){
    var t=dt?dt:toType(x);
    switch(t){
      case'null':return 2;
      case'object':return 1+calcN(getKeys(x),'symbols')+calcN(getVals(x),null);
      case'boolean':return 2;
      case'number':return 9;
      case'array':{var n=6;for(var i=0;i<x.length;i++)n+=calcN(x[i],null);return n;}
      case'symbols':{var n=6;for(var i=0;i<x.length;i++)n+=calcN(x[i],'symbol');return n;}
      case'string':return CdotJS.u8u16(x).length+(x[0]=='`'?1:6);
      case'date':return 9;
      case'symbol':return 2+CdotJS.u8u16(x).length;}
    throw "bad type "+t;}
  function wb(b){ub[pos++]=b;}
  function wn(n){for(var i=0;i<n;i++)ub[pos++]=bb[i];}
  function w(x,dt){
    var t=dt?dt:toType(x);
    switch(t){
      case 'null':{wb(101);wb(0);}break;
      case 'boolean':{wb(-1);wb(x?1:0);}break;
      case 'number':{wb(-9);fb[0]=x;wn(8);}break;
      case 'date':{wb(-15);fb[0]=((x.getTime()-(new Date(x)).getTimezoneOffset()*60000)/86400000)-10957;wn(8);}break;
      case 'symbol':{wb(-11);x=CdotJS.u8u16(x);for(var i=0;i<x.length;i++)wb(x[i]);wb(0);}break;
      case 'string':if(x[0]=='`'){w(x.substr(1),'symbol');}else{wb(10);wb(0);x=CdotJS.u8u16(x);ib[0]=x.length;wn(4);for(var i=0;i<x.length;i++)wb(x[i]);}break;
      case 'object':{wb(99);w(getKeys(x),'symbols');w(getVals(x),null);}break;
      case 'array':{wb(0);wb(0);ib[0]=x.length;wn(4);for(var i=0;i<x.length;i++)w(x[i],null);}break;
      case 'symbols':{wb(0);wb(0);ib[0]=x.length;wn(4);for(var i=0;i<x.length;i++)w(x[i],'symbol');}break;}}
  var n=calcN(x,null);
  var ab=new ArrayBuffer(8+n);
  ub=new Uint8Array(ab);
  wb(1);wb(0);wb(0);wb(0);ib[0]=ub.length;wn(4);w(x,null);
  return ab;
}


