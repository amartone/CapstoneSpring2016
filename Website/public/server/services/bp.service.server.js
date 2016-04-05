/**
 * Created by Andrew on 3/15/16.
 */
module.exports = function (app, bpModel) {
    app.get("/api/capstone/user/:userId/bp", getAllBpForUser);

    function getAllBpForUser(req, res) {
        var userId = req.params.userId;

        userId = bpModel.findBpByUserId(userId)
            .then(
                function ( doc ) {
                    console.log("dsfdsfdsfd: " + doc);
                    res.json(doc);
                },
                // send error if promise rejected
                function ( err ) {
                    console.log(err);
                    res.status(400).send(err);
                }
            );
    }
};