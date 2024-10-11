import React from 'react'
import Table from './Table'
import * as Mg from './kdb/ipc'


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
    this.conn = new Mg.MgKdb.Connection('ws://localhost:30098', this.listener)
  }
  componentWillUnmount = () => {
    this.conn.close()
  }
  onTableResponse = (isErr, data) => {
    this.setState({table: data})
  }
  kdbConnected = conn => {
    this.setState({connected: true})
    var func = Mg.KdbCharVector.fromString(".web.getTable")
    var arg0 = Mg.KdbTimeUtil.Time.atomFromJsDate(new Date())
    var arg1 = Mg.KdbTimeUtil.Timestamp.atomFromJsDate(new Date())
    var request = new Mg.KdbList([func, arg0, arg1])
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