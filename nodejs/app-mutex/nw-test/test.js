var assert = require('assert');
var mutex = require('./app-mutex');

function log(/*...*/) {
    var div = document.createElement("div");
    div.style.textAlign = "right";
    var text = "[empty string]";
    if (arguments.length) {
        text = typeof(arguments[0]) == "undefined" ? "undefined" : arguments[0].toString();
        for (var i = 1; i < arguments.length; i++) {
            text += " " + (typeof(arguments[i]) == "undefined" ? "undefined" : arguments[i]);
        }
    }
    div.appendChild(document.createTextNode(text));
    document.getElementById("output").appendChild(div);
}

var guid = (function() {
  function s4() {
    return Math.floor((1 + Math.random()) * 0x10000)
               .toString(16)
               .substring(1);
  }
  return function() {
    return s4() + s4() + '-' + s4() + '-' + s4() + '-' +
           s4() + '-' + s4() + s4() + s4();
  };
})();

// ==================================
// Start the test
// ==================================
var name1 = guid();
log("Starting the test with guid", name1);

// Test create()
var id1 = mutex.create(name1);
assert.equal(id1, 1, "Creation: could not create a unique global mutex");

// Test getName()
assert.equal(name1, mutex.getName(id1), "getName() failed to get the correct mutex name");

// Test create() to not make new mutex with same name
assert.equal(mutex.create(name1), 0, "Creation: using the same name creates a new mutex.");

// Test create() with different name
var name2 = guid();
var id2 = mutex.create(name2);
assert.equal(id2, 2, "Creation: could not create another unique global mutex.");

// Test getName() with another name
assert.equal(name2, mutex.getName(id2), "getName() failed to get the correct mutex name again");

// Test getName() with non-existent name
assert.equal(mutex.getName(10), null, "getName() failed and somehow got a name with a non-existent id");

// Test destroy() and getName() on it
mutex.destroy(id2);
assert.equal(mutex.getName(id2), null, "getName() failed after destroying an existent id");

// Test that create() increments the id even if previous was destroyed
assert.equal(mutex.create(guid()), id2 + 1, "create() did not increment the ids.");

log("All tests passed");