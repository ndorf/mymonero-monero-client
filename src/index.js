const WABridge = require('./WABridge')

module.exports = function () {

  return new Promise(function (resolve, reject) {
    require('./MyMoneroClient_WASM')({}).then(function (thisModule) {
      const instance = new WABridge(thisModule)
      resolve(instance)
    }).catch(function (e) {
      console.error('Error loading MyMoneroClient_WASM:', e)
      reject(e)
    })
  })
}
