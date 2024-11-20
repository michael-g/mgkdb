/*
This file is part of the Mg KDB-IPC Javascript Library (hereinafter "The Library").

The Library is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

The Library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Affero Public License for more details.

You should have received a copy of the GNU Affero Public License along with The
Library. If not, see https://www.gnu.org/licenses/agpl.txt.
*/
import React from 'react'
import { TypeWrapper } from './Types'
import { MgKdb, C, } from './ipc'
import {
  KdbCharVector, KdbTimestampAtom, KdbMonthAtom, KdbDateAtom, KdbTimespanAtom, 
  KdbMinuteAtom, KdbSecondAtom, KdbTimeAtom, KdbList
} from './ipc'

import controls from './ControlsPane.module.css'
import results from './ResultPane.module.css'


class ResultPane extends React.PureComponent {

  renderObject = () => {
    const obj = this.props.object
    return !obj ? <></> : <TypeWrapper object={obj}/>
  }

  render = () => {
    return (
      <div className={results.resultPane}>
        {this.renderObject()}
      </div>
    )
  }
}

export default class AppRoot extends React.Component {
  constructor(props) {
    super(props)
    this.conn = null
    this.state = {
      connected: false,
      obj: null,
    }
  }

  componentDidMount = () => {
    this.listener = {
      onConnected: this.kdbConnected,
      onDisconnected: this.kdbDisconnected,
    }
    this.conn = new MgKdb.Endpoint('ws://localhost:30098', this.listener)
  }

  componentWillUnmount = () => {
    this.conn.close()
  }

  onResponse = (isErr, data) => {
    this.setState({obj: data})
  }

  makeTable1 = () => {
    const now = new Date()
    var func = new KdbCharVector(".web.getTable")
    var timestamp = new KdbTimestampAtom(now)
    var month = new KdbMonthAtom(now)
    var date = new KdbDateAtom(now)
    var timespan = new KdbTimespanAtom(now)
    var minute = new KdbMinuteAtom(now)
    var second = new KdbSecondAtom(now)
    var millis = new KdbTimeAtom(now)
    var arg0 = new KdbList([timestamp, month, date, timespan, minute, second, millis])
    return new KdbList([func, arg0])
  }

  makeTable = () => {
    const now = new Date()
    var fun = C.ks(".web.getTable")

    var timestamp = C.kp(now)
    var month = C.km(now)
    var date = C.kd(now)
    var timespan = C.kn(now)
    var minute = C.ku(now)
    var second = C.kv(now)
    var millis = C.kt(now)

    var names = C.kcv('pmdnuvt')
    var times = C.kva(timestamp, month, date, timespan, minute, second, millis)
    var dct = C.xD(names, times)

    var time = C.ktv(19, now, now)
    var syms = C.ksv('VOD.L', 'GSK.L')
    var prcs = C.ktv(9, 99.1, 99.2)
    var size = C.ktv(7, 7n, 8n)

    var vals = C.kva(time, syms, prcs, size)
    var cols = C.ksv('time', 'sym', 'price', 'size')

    var tbl = C.xT(cols, vals)
    
    return C.kva(fun, C.kva(tbl, dct))
  }

  requestTable = () => C.kva(C.ks(".web.getTable"), C.ki(42))

  requestDict = () => C.kva(C.ks('.web.getDict'))
  
  requestCharVec = () => C.kva(C.ks(".web.getCharVec"))

  requestSymAtom = () => C.kva(C.ks(".web.getSymAtom"))

  kdbConnected = conn => {
    this.setState({connected: true})
    this.setType(98)
  }

  kdbDisconnected = conn => {
    this.setState({connected: false})
  }

  setType = typ => {
    var request
    switch (typ) {
      case  99: request = this.requestDict(); break
      case  98: request = this.requestTable(); break
      case  10: request = this.requestCharVec(); break
      case -11: request = this.requestSymAtom(); break

      default:
        console.warn("WARNING: setType has no handler for type %d", typ)
        return
    }
    if (request) this.conn.sendRequest(request, this.onResponse)
  }

  evalInput = evt => {
    evt.preventDefault()
    evt.stopPropagation()
    const cmd = evt.target.parentNode.elements['qinput'].value
    console.log(cmd)
    this.conn.sendRequest(C.kva(C.ks('.web.evalCmd'), C.kcv(cmd)), this.onResponse)
  }

  render = () => {
    return (<>
      <div>
        <div className={controls.controlsPane}>
          <button onClick={()=>this.setType(98)}>Table</button>
          <button onClick={()=>this.setType(99)}>Dict</button>
          <button onClick={()=>this.setType(10)}>Char-vec</button>
          <button onClick={()=>this.setType(-11)}>Sym-Atom</button>
          <form>
            <textarea name='qinput' className={controls.qInput}>.z.p</textarea>
            <button onClick={this.evalInput}>GO</button>
          </form>
        </div>
        <ResultPane object={this.state.obj}/>
      </div>
    </>)
  }
}
