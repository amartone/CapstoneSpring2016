module.exports = function(app, db, mongoose) {
    var userModel = require("./models/user.model.server.js")( db, mongoose);
    var bpModel = require("./models/bp.model.server.js")( db, mongoose);

    var userService = require("./services/user.service.server.js") (app, userModel);
    var bpService = require("./services/bp.service.server.js") (app, bpModel);

};