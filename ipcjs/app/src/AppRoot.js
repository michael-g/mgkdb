import React from 'react'

import * as Mg from './kdb/ipc'

export default class AppRoot extends React.Component {
  constructor(props) {
    super(props)
    this.conn = null
    this.state = {
      connected: false,
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
  onHostNameResponse = (isErr, data) => {
    console.log("hostname: " + data)
  }
  kdbConnected = conn => {
    this.setState({connected: true})
    this.conn.sendRequest(Mg.KdbCharVector.fromString(".z.h"), this.onHostNameResponse)
  }
  kdbDisconnected = conn => {
    this.setState({connected: false})
  }
  render = () => {
    return <>Hello, World!</>
  }
}