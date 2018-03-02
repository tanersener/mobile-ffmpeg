// libshine.js - port of libshine to JavaScript using emscripten
// by Romain Beauxis <toots@rastageeks.org> from code by
// Andreas Krennmair <ak@synflood.at>


var Shine = (function() {
  var Module;
  var context = {};
  return (function() {
