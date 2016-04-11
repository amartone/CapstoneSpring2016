/**
 * Created by Andrew on 3/15/16.
 */
(function(){
    angular
        .module("CapstoneApp")
        .factory("BPService", bpService);

    function bpService($http, $rootScope) {
        var api = {
            findAllBpForUser: findAllBpForUser,
            getSampleDataForUser: getSampleDataForUser

        };

        return api;

        function findAllBpForUser(userId){
            return $http.get("/api/capstone/user/" + userId + "/bp");
        }

        function getSampleDataForUser(userId){
            return $http.get("/api/capstone/user/" + userId + "/sampledata");
        }

    }
})();
