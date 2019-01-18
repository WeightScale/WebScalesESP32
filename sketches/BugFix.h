/******************************************Ошибка в AsyncWebSocket.cpp*************************************************************
 *	
 *	когда обрывается связь выключением WiFi на стороне клиента в следующих методах _client is Null 
 *	void AsyncWebSocketClient::_queueControl(AsyncWebSocketControl *controlMessage)
 *	void AsyncWebSocketClient::_queueMessage(AsyncWebSocketMessage *dataMessage)
 *	
 *исправить методы
 *
 *void AsyncWebSocketClient::_queueMessage(AsyncWebSocketMessage *dataMessage){	
	if(dataMessage == NULL)
		return;
	if (!_client){
		delete dataMessage;
		return;
	}
	if(_status != WS_CONNECTED){
		delete dataMessage;
		return;
	}
  if(_messageQueue.length() > WS_MAX_QUEUED_MESSAGES){
      ets_printf("ERROR: Too many messages queued\n");
      delete dataMessage;
  } else {
      _messageQueue.add(dataMessage);
  }
  if(_client->canSend())
    _runQueue();
}
 *	
 *
 *void AsyncWebSocketClient::_queueControl(AsyncWebSocketControl *controlMessage){	
	if(controlMessage == NULL)
		return;
	if (!_client){
		delete controlMessage;
		return;
	}
	_controlQueue.add(controlMessage);
	if(_client->canSend())
		_runQueue();
}
 *************************************************************************************/