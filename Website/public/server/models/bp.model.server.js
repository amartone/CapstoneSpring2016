/**
 * Created by Andrew on 3/15/16.
 */

var q = require("q");
module.exports = function (db, mongoose) {

    var BPSchema = require("./bp.schema.server.js")(mongoose);
    var SampleSchema = require("./sample.schema.server.js")(mongoose);

    // create user model from schema
    var BPModel = mongoose.model('BP', BPSchema);
    var SampleModel = mongoose.model('S', SampleSchema);


    var api = {
        findBpByUserId: findBpByUserId,
        importIntoMongo: importIntoMongo
    };
    return api;


    function findBpByUserId(userId) {
        var deferred = q.defer();

        // find one retrieves one document
        BPModel.find({userId: userId}, function (err, doc) {
            if (err) {
                deferred.reject(err);
            } else {
                console.log("Found:" + doc)
                deferred.resolve(doc);
            }
        });
        return deferred.promise;
    }

    function importIntoMongo(sample){
        var deferred = q.defer();


        SampleModel.create(sample, function (err, doc) {
            if (err) {
                deferred.reject(err);
            } else {
                deferred.resolve(doc);
            }
        });
        return deferred.promise;
    }

};