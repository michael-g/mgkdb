import React from 'react'

import * as Mg from './kdb/ipc'

import Table from './Table'

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
    console.log("onTableResponse " + data)
    this.setState({table: data})
  }
  kdbConnected = conn => {
    this.setState({connected: true})
    this.conn.sendRequest([Mg.KdbCharVector.fromString(".web.getTable")], this.onTableResponse)
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