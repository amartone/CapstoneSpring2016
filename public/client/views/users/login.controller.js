/**
 * Created by Andrew on 2/23/16.
 */
/**
 * Created by Andrew on 2/23/16.
 */
(function () {
    angular
        .module("CapstoneApp")
        .controller("LoginController", LoginController);

    function LoginController(UserService, $location) {
        var vm = this;
        vm.findUserByCredentials = findUserByCredentials;

        function init() {
        }

        init();

        function findUserByCredentials(user, pass) {
            if (!user) {
                return;
            }
            UserService
                .findUserByCredentials(
                    user,
                    pass
                )
                .then(function (response) {
                    if (response.data) {
                        UserService.setCurrentUser(response.data);
                        $location.url("/profile");
                    }
                });
        }
    }
})();