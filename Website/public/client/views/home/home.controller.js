/**
 * Created by Andrew on 2/23/16.
 */
(function(){
    angular
        .module("CapstoneApp")
        .controller("HomeController", HomeController);

    function HomeController (BPService, $rootScope) {


        var vm = this;

        function init() {
            BPService.findAllBpForUser($rootScope.currentUser._id)
                .then(function(response){
                    console.log(response.data);
                    vm.bp=response.data;

                });
        }
        init();

    }

})();