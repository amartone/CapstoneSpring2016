/**
 * Created by Andrew on 2/24/16.
 */
(function(){
    angular
        .module("CapstoneApp")
        .config(Configure);

    function Configure($routeProvider) {
        $routeProvider
            .when("/home",{
                templateUrl: "views/home/home.view.html",
                controller: "HomeController",
                resolve:{
                    checkLoggedIn: checkLoggedIn
                }
            })
            .when("/welcome",{
                templateUrl: "views/home/welcome.view.html",
                resolve:{
                    checkLoggedIn: checkWelcome
                }
            })
            .when("/register", {
                templateUrl: "views/users/register.view.html",
                controller: "RegisterController",
                controllerAs: "model"
            })
            .when("/login", {
                templateUrl: "views/users/login.view.html",
                controller: "LoginController",
                controllerAs: "model"

            })
            .when("/profile/", {
                templateUrl: "views/users/profile.view.html",
                controller: "ProfileController",
                controllerAs: "model",
                resolve:{
                    checkLoggedIn: checkLoggedIn
                }
            })
            .otherwise({
                redirectTo: "/welcome"
            });
    }
    function getLoggedIn(UserService, $q) {
        var deferred = $q.defer();

        UserService
            .getCurrentUser()
            .then(function(response){
                var currentUser = response.data;
                UserService.setCurrentUser(currentUser);
                deferred.resolve();
            });

        return deferred.promise;
    }

    function checkWelcome(UserService, $q, $location){
        var deferred = $q.defer();

        UserService.getCurrentUser()
            .then(function(response) {
                var currentUser = response.data;

                if(currentUser) {
                    deferred.reject();
                } else {
                    deferred.resolve();
                    $location.url("/welcome");
                }
            });

        return deferred.promise;
    }


    function checkLoggedIn(UserService, $q, $location) {

        var deferred = $q.defer();

        UserService.getCurrentUser()
            .then(function(response) {
                var currentUser = response.data;

                if(currentUser) {
                    UserService.setCurrentUser(currentUser);
                    deferred.resolve();
                } else {
                    deferred.reject();
                    $location.url("/welcome");
                }
            });

        return deferred.promise;
    }

})();