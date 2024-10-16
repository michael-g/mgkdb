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
    if (now.getTime() % 2) {
      var func = new Mg.KdbCharVector(".web.getTable")
      var timestamp = new Mg.KdbTimestampAtom(now)
      var month = new Mg.KdbMonthAtom(now)
      var date = new Mg.KdbDateAtom(now)
      var timespan = new Mg.KdbTimespanAtom(now)
      var minute = new Mg.KdbMinuteAtom(now)
      var second = new Mg.KdbSecondAtom(now)
      var time = new Mg.KdbTimeAtom(now)
      var arg0 = new Mg.KdbList([timestamp, month, date, timespan, minute, second, time])
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
      var time = C.kt(now)
      request = C.knk(fun, C.knk(timestamp, month, date, timespan, minute, second, time))
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