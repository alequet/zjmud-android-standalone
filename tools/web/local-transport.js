/* global zjmudNative */
(function (global) {
  'use strict';

  var listeners = {};
  var pendingEvents = {};

  function emitLocal(event, payload) {
    var handlers = listeners[event] || [];
    if (!handlers.length) {
      if (!pendingEvents[event]) pendingEvents[event] = [];
      pendingEvents[event].push(payload);
      return;
    }
    handlers.slice().forEach(function (handler) {
      handler(payload);
    });
  }

  var socket = {
    on: function (event, handler) {
      if (!listeners[event]) listeners[event] = [];
      listeners[event].push(handler);
      var pending = pendingEvents[event] || [];
      delete pendingEvents[event];
      pending.forEach(function (payload) {
        handler(payload);
      });
      return socket;
    },
    emit: function (event, payload) {
      zjmudNative.postMessage(JSON.stringify({
        type: 'emit',
        event: event,
        payload: payload == null ? '' : String(payload)
      }));
      return socket;
    }
  };

  global.localTransport = {
    connect: function () {
      return socket;
    }
  };

  zjmudNative.onmessage = function (message) {
    try {
      var data = JSON.parse(message.data);
      emitLocal(data.event, data.payload || '');
    } catch (error) {
      emitLocal('status', '本地通信数据无效。\n');
    }
  };

  zjmudNative.postMessage(JSON.stringify({ type: 'ready' }));
})(window);
