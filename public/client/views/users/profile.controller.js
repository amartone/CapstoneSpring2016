/**
 * Created by Andrew on 2/23/16.
 */
(function () {
    angular
        .module("CapstoneApp")
        .controller("ProfileController", ProfileController);

    function ProfileController(UserService, $rootScope, $location) {

        var vm = this;
        vm.update = update;

        function init() {
            UserService.getCurrentUser()
                .then(function(response){
                    vm.user=response.data;
                    console.log(vm.user.emails);
                });
        }

        init();

        function update(user) {
            console.log(user.username);
            console.log(user._id);

            var emails = user.emails.toString().split(",");
            user.emails = emails;

            var phones = user.phones.toString().split(",");
            user.phones = phones;


            UserService
                .updateUser(user._id, user)
                .then(function (response) {
                    var currentUser = response.data;
                    if(currentUser){
                        console.log(currentUser);
                        UserService.setCurrentUser(currentUser);
                        $location.url("/profile");
                    }
                });
        }
    }
})();