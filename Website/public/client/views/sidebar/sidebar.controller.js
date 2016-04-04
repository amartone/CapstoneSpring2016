/**
 * Created by Andrew on 2/24/16.
 */
(function(){
    angular
        .module("CapstoneApp")
        .controller("SidebarController", SidebarController);

    function SidebarController($location, $scope) {
        console.log($location);
        $scope.$location = $location;
    }
})();

