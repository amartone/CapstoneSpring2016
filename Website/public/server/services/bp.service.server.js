/**
 * Created by Andrew on 3/15/16.
 */
module.exports = function (app, bpModel) {

    app.get("/api/capstone/user/:userId/bp", getAllBpForUser);
    app.get("/api/capstone/user/:userId/sampledata", getSampleDataForUser);

    app.post("/api/capstone/bp/", importIntoMongo);


    function getAllBpForUser(req, res) {
        var userId = req.params.userId;

        userId = bpModel.findBpByUserId(userId)
            .then(
                function ( doc ) {
                    res.json(doc);
                },
                // send error if promise rejected
                function ( err ) {
                    console.log(err);
                    res.status(400).send(err);
                }
            );
    }


    function importIntoMongo(req, res){
        var sample = req.body;

        bpModel.importIntoMongo(sample)
            .then(
                function(doc){
                    res.json(doc);
                },

                function(err){
                    res.status(400).send(err);
                }
            )


    }

    function getSampleDataForUser(req, res){
        var userId = req.params.userId;

        bpModel.getSampleDataForUser(userId)
            .then(
                function(doc){
                    res.json(doc);
                },

                function(err){
                    res.status(400).send(err);
                }
            )



    }
};
