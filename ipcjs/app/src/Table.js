import React from 'react'

import { KdbTable, KdbTimeUtil, KdbByteAtom } from './kdb/ipc'

import styles from './Table.module.css'

class Vector extends React.PureComponent {
  renderElem = (val, idx) => <div key={idx}>{val}</div>
  render = () => {
    const {vec, idcs} = this.props
    const ary = new Array(vec.ary.length)
    for (let i = 0 ; i < ary.length ; i++) {
      ary[i] = this.renderElem(vec.ary[idcs[i]], i)
    }
    return ary;
  }
}

class BoolVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>{val}</div>
}
class ByteVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>0x{KdbByteAtom.toHexDigits(val)}</div>
}
class IntegerVector extends Vector {
  static F = new Intl.NumberFormat('en-GB', {minimumIntegerDigits: 1, useGrouping: true })
  renderElem = (val, idx) => <div key={idx} className={styles.integer}>{IntegerVector.F.format(val)}</div>
}
class FloatingPointVector extends Vector {
  static F = new Intl.NumberFormat('en-GB', {minimumIntegerDigits: 1, minimumFractionDigits: 3, maximumFractionDigits: 3, useGrouping: true })
  renderElem = (val, idx) => <div key={idx} className={styles.fraction}>{FloatingPointVector.F.format(val)}</div>
}
class CharVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>{String.fromCharCode(val)}</div>
}
class TimestampVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>{KdbTimeUtil.Timestamp.toString(val)}</div>
}
class MonthVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>{KdbTimeUtil.Month.toString(val)}</div>
}
class DateVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>{KdbTimeUtil.Date.toString(val)}</div>
}
class TimespanVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>{KdbTimeUtil.Timespan.toString(val)}</div>
}
class MinuteVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>{KdbTimeUtil.Minute.toString(val)}</div>
}
class SecondVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>{KdbTimeUtil.Second.toString(val)}</div>
}
class TimeVector extends Vector {
  renderElem = (val, idx) => <div key={idx}>{KdbTimeUtil.Time.toString(val)}</div>
}
class List extends Vector {
  renderElem = (val, idx) => <div key={idx}>{val.toString()}</div>
}

const TYPE_MAP = {
  0: List,
  1: BoolVector,
  4: ByteVector,
  5: IntegerVector,
  6: IntegerVector,
  7: IntegerVector,
  8: FloatingPointVector,
  9: FloatingPointVector,
  10: CharVector,
  12: TimestampVector,
  13: MonthVector,
  14: DateVector,
  16: TimespanVector,
  17: MinuteVector,
  18: SecondVector,
  19: TimeVector,
}

/**
tbl
cfg

cfg.cols.names:
 array of names conforming to `cols tbl`
cfg.cols.titles:
 object of key:rename or key:function(kColName, idx) to override table header
cfg.cols.formatter:
  object of key:Component or key:function(kColName, colIdx, kVector, rowIdx)
 */
class Table extends React.PureComponent {

  static anchorId = 0

  constructor(props) {
    super(props)
    this.anchorName = `--table-${Table.anchorId++}-col-`
    this.state = {
      widths:{},
      sortBy:[],
      sortIdcs: null,
    }
  }

  getColVecPairs = () => {
    const {cfg, tbl} = this.props
    if (cfg.cols && cfg.cols.names) {
      const nms = cfg.cols.names
      const vls = tbl.vals.ary
      const ary = new Array(nms.length)
      for (let i = 0 ; i < nms.length ; i++) {
        const idx = tbl.findColIdx(nms[i])
        if (-1 === idx) {
          throw Error(`Column '${nms[i]}' is not in col-set ${tbl.keys}`)
        }
        ary[i]= [nms[i], vls[i]]
      }
      return ary
    }
    return tbl.cols.ary.map((col, idx) => [col, tbl.vals.ary[idx]])
  }
 

  getTitle = (cfg, col, idx) => {
    if (cfg.cols && cfg.cols.titles && col in cfg.cols.titles) {
      const tls = cfg.cols.titles
      if (typeof(tls[col]) === 'function') {
        return tls[col](col, idx)
      }
      return tls[col] // expected to be renderable
    }
    return col
  }

  onDragStart = (col, hdr, evt) => {
    this.dragStartX = evt.pageX;
  }

  onDragEnd = (col, hdr, evt) => {
    this.dragStopX = evt.pageX;
    const len = this.dragStopX - this.dragStartX
    const width = hdr.getBoundingClientRect().width + len
    if (width > 10) {
      this.setState({widths: Object.assign({}, this.state.widths, {[col]: width})})
    }
  }

  sortGenAsc = ([lhs, _i], [rhs, _j]) => lhs == rhs ? 0 : lhs < rhs ? -1 : 1

  sortNumAsc = ([lhs, _i], [rhs, _j]) => Number(lhs - rhs)

  onSortClick = evt => {
    const tbl = this.props.tbl
    const col = evt.target.attributes['data-kcol-name'].value
    const idx = tbl.findColIdx(col)
    const vec = tbl.vals.ary[idx]
    const src = vec.ary
    const dst = new Array(src.length)
    // TODO if previously sorted, use this.state.sortIdcs
    for (let i = 0 ; i < src.length ; i++) {
      dst[i] = [src[i], i]
    }
    let sortAsc = true
    var idcs
    if (sortAsc) {
      var fun = this.sortGenAsc
      if (vec.typ === 1 || (vec.typ >= 4 && vec.typ <= 9) || (vec.typ >= 12 && vec.typ <= 19)) {
        fun = this.sortNumAsc
      }
      idcs = dst.sort(fun).map(([_, idx]) => idx)
    }
    this.setState({sortIdcs: idcs})
  }

  renderHeader = (cfg, col, idx) => {
    const title = this.getTitle(cfg, col, idx)
    const name = this.anchorName + col
    const ref = React.createRef()
    const colOpts = {
      style: {anchorName: name},
      onClick: this.onSortClick,
      ['data-kcol-name']: col,
      ref: ref,
    }
   
    const dragOpts = {
      draggable: true,
      style: {positionAnchor: name},
      onDragStart: evt => this.onDragStart(col, ref.current, evt),
      onDragEnd: evt => this.onDragEnd(col, ref.current, evt),
    }
    return (<>
      <div key={-1} {...colOpts}>{title}</div>
      <div className={styles.headerHandle} {...dragOpts}>&nbsp;</div>
    </>)
  }

  renderVals = (cfg, idc, col, idx, val) => {
    if (cfg.cols && cfg.cols.formatter && col in cfg.cols.colfmtr) {
      return cfg.cols.colfmtr(col, val)
    }
    if (cfg.cols && cfg.cols.formatter && col in cfg.cols.rowfmtr) {
      const fmt = cfg.cols.rowfmtr
      const ary = new Array(val.ary.length)
      
      for (let i = 0 ; i < ary.length ; i++) {
        ary[i] = fmt(col, val, idc[i])
      }
      return ary
    }
    const Vec = !(val.typ in TYPE_MAP) ? Vector : TYPE_MAP[val.typ]
    return <Vec vec={val} col={col} col_idx={idx} idcs={idc}/>
  }

  renderCol = (cfg, idc, idx, pair) => {
    const [col, val] = pair
    const opts = {}
    if (col in this.state.widths) {
      const width = `${this.state.widths[col]}px`
      opts.style = {width, maxWidth: width, minWidth: width}
    }
    return (
      <div key={col} className={styles.column} {...opts}>
        {this.renderHeader(cfg, col, idx)}
        {this.renderVals(cfg, idc, col, idx, val)}
      </div>
    )
  }
  
  render = () => {
    const {tbl, cfg} = this.props
    if (tbl.constructor !== KdbTable) {
      return <></>
    }
    const cols = this.getColVecPairs()
    let idcs = this.state.sortIdcs
    idcs = idcs !== null ? idcs : Array.from(Array(tbl.vals.ary[0].count()).keys())
    
    return (
      <div className={styles.table}>
        {cols.map((pair, idx) => this.renderCol(cfg, idcs, idx, pair))}
      </div>
    )
  }

}

export default Table