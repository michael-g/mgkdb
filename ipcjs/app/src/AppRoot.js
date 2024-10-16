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
import Table from './Table'
import * as Mg from './kdb/ipc'
import { C } from './kdb/ipc'


export default class AppRoot extends React.Component {
  constructor(props) {
    super(props)
    this.conn = null
    this.state = {
      connected: false,
      table: null,
    }
  }
  componentDidMount = () => {
    this.listener = {
      onConnected: this.kdbConnected,
      onDisconnected: this.kdbDisconnected,
    }
    this.conn = new Mg.MgKdb.Endpoint('ws://localhost:30098', this.listener)
  }
  componentWillUnmount = () => {
    this.conn.close()
  }
  onTableResponse = (isErr, data) => {
    this.setState({table: data})
  }
  kdbConnected = conn => {
    this.setState({connected: true})
    const now = new Date()
    var request
    if (false) {
      var func = new Mg.KdbCharVector(".web.getTable")
      var timestamp = new Mg.KdbTimestampAtom(now)
      var month = new Mg.KdbMonthAtom(now)
      var date = new Mg.KdbDateAtom(now)
      var timespan = new Mg.KdbTimespanAtom(now)
      var minute = new Mg.KdbMinuteAtom(now)
      var second = new Mg.KdbSecondAtom(now)
      var millis = new Mg.KdbTimeAtom(now)
      var arg0 = new Mg.KdbList([timestamp, month, date, timespan, minute, second, millis])
      request = new Mg.KdbList([func, arg0])
    }
    else {
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
      
      request = C.kva(fun, C.kva(tbl, dct))
    }
    this.conn.sendRequest(request, this.onTableResponse)
  }
  kdbDisconnected = conn => {
    this.setState({connected: false})
  }
  render = () => {
    const table = this.state.table
    if (table === null) return <>Hello, World!</>
    return <Table tbl={table} cfg={{}}/>
  }
}