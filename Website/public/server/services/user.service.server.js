/**
 * Created by Andrew on 3/15/16.
 */
module.exports = function (app, userModel) {
    app.post("/api/assignment/user", register);
    app.get("/api/assignment/user/:id", findUserById);
    app.put("/api/assignment/user/:id", updateUser);
    //app.get("/api/assignment/user?username=username", findUserByUsername);
    app.get("/api/assignment/user", apiRouter);
    app.delete("/api/assignment/user/:id", deleteUser);
    app.post("/api/assignment/logout", logout);


    app.get("/api/assignment/loggedin", loggedIn);


    function logout(req, res) {
        req.session.currentUser = null;
        console.log(req.session.currentUser);
        res.json(req.session.currentUser);
    }


    function loggedIn(req, res) {
        console.log(req.session.currentUser);
        res.json(req.session.currentUser);
    }

    function apiRouter(req, res) {
        if (req.query.username && req.query.password) {
            findUserByCredentials(req, res);
        }
        else if (req.query.username) {
            findUserByUsername(req, res);
        } else {
            findAllUsers(req, res);
        }
    }

    function updateUser(req, res) {
        var userId = req.params.id;
        var user = req.body;

        user = userModel.updateUser(userId, user)
            .then(function (doc) {
                    req.session.currentUser = doc;
                    res.json(doc);
                },
                function (err) {
                    res.status(400).send(err);
                }
            );
    }

    function register(req, res) {
        var user = req.body;
        user = userModel.createUser(user)
            // handle model promise
            .then(
                // login user if promise resolved
                function (doc) {
                    req.session.currentUser = doc;
                    res.json(doc);
                },
                // send error if promise rejected
                function (err) {
                    res.status(400).send(err);
                }
            );
    }


    function findAllUsers(req, res) {
        var users = userModel.findAllUsers();
        res.json(users);
    }

    function findUserById(req, res) {
        var userId = req.params.userId;

        var user = userModel.findUserById(userId)
            .then(
                // return user if promise resolved
                function (doc) {
                    res.json(doc);
                },
                // send error if promise rejected
                function (err) {
                    res.status(400).send(err);
                }
            );
    }

    function findUserByUsername(req, res) {
        var username = req.params.username;
        var user = userModel.findByUsername(username);
        res.json(user);
    }

    function findUserByCredentials(req, res) {
        var username = req.query.username;
        var password = req.query.password;

        var user = userModel.findUserByCredentials(username, password)
            .then(
                function (doc) {
                    req.session.currentUser = doc;
                    res.json(doc);
                },
                // send error if promise rejected
                function (err) {
                    res.status(400).send(err);
                }
            );
    }

    function deleteUser(req, res) {
        var userId = req.params.id;
        userModel.deleteUser(userId);
        res.send(200);
    }

    //function logout(req, res) {
    //    userModel.setCurrentUser(null);
    //}

}
