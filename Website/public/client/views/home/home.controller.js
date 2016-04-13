/**
 * Created by Andrew on 2/23/16.
 */
(function(){
    angular
        .module("CapstoneApp")
        .controller("HomeController", HomeController);

    function HomeController (BPService, $rootScope) {


        var vm = this;
        var drawDashboard = drawDashboard
        // Set a callback to run when the Google Visualization API is loaded.

        function init() {
            BPService.findAllBpForUser($rootScope.currentUser._id)
                .then(function(response){
                    console.log(response.data);
                    vm.bp=response.data;
                });

                BPService.getSampleDataForUser($rootScope.currentUser._id)
                    .then(function(response){
                        console.log("datfdfda:" + response.data);
                        vm.sample_data=response.data;
                        console.log(vm.sample_data[0])
                        google.charts.setOnLoadCallback(drawDashboard);

                    });
        }
        init();

        function drawDashboard() {

        // Create our data table.
        //   var data = new google.visualization.DataTable();
        // data.addColumn('number', 'X');
        // data.addColumn('number', 'Dogs');
        //
        // data.addRows([
        //   [0, 0],   [1, 10],  [2, 23],  [3, 17],  [4, 18],  [5, 9],
        //   [6, 11],  [7, 27],  [8, 33],  [9, 40],  [10, 32], [11, 35],
        //   [12, 30], [13, 40], [14, 42], [15, 47], [16, 44], [17, 48],
        //   [18, 52], [19, 54], [20, 42], [21, 55], [22, 56], [23, 57],
        //   [24, 60], [25, 50], [26, 52], [27, 51], [28, 49], [29, 53],
        //   [30, 55], [31, 60], [32, 61], [33, 59], [34, 62], [35, 65],
        //   [36, 62], [37, 58], [38, 55], [39, 61], [40, 64], [41, 65],
        //   [42, 63], [43, 66], [44, 67], [45, 69], [46, 69], [47, 70],
        //   [48, 72], [49, 68], [50, 66], [51, 65], [52, 67], [53, 70],
        //   [54, 71], [55, 72], [56, 73], [57, 75], [58, 70], [59, 68],
        //   [60, 64], [61, 60], [62, 65], [63, 67], [64, 68], [65, 69],
        //   [66, 70], [67, 72], [68, 75], [69, 80]
        // ]);



        // var idata = {"userId":"570373d678c7bb83a444392e",
        // "measurements":[{ "impedance_magnitude" : 773.5, "impedance_phase" : -15.4375, "pressure" : 49},
        // { "impedance_magnitude" : 773.5, "impedance_phase" : -15.4375, "pressure" : 50 },
        // { "impedance_magnitude" : 773.375, "impedance_phase" : -15.4375, "pressure" : 49},
        // { "impedance_magnitude" : 773.375, "impedance_phase" : -15.4375, "pressure" : 49},
        // { "impedance_magnitude" : 773.375, "impedance_phase" : -15.4375, "pressure" : 49},
        // { "impedance_magnitude" : 773.4375, "impedance_phase" : -15.4375, "pressure" : 49},
        // { "impedance_magnitude" : 773.4375, "impedance_phase" : -15.4375, "pressure" : 49}]
        // }

        var sampleNum = 0
        var dataArray = [["Samples", "Impedance"]];
        for (var i = 0; i<vm.sample_data[0].measurements.length; i++){
          var obj = vm.sample_data[0].measurements[i];
          sampleNum+= 1;
          imp = obj.impedance_magnitude;
          x = [sampleNum, imp];
          dataArray.push(x);

        }

        var data = new google.visualization.arrayToDataTable(dataArray)


          // Create a dashboard.
          var dashboard = new google.visualization.Dashboard(
              document.getElementById('dashboard_div'));

          // Create a range slider, passing some options
          var impedanceRangeSlider = new google.visualization.ControlWrapper({
            'controlType': 'ChartRangeFilter',
            'containerId': 'filter_div',
            'options': {
              'filterColumnLabel': 'Samples'
            }
          });

          // Create a line chart, passing some options
          var impedanceChart = new google.visualization.ChartWrapper({
            'chartType': 'LineChart',
            'containerId': 'chart_div',
            'options': {
                'hAxis': {
                    'title': 'Samples'
                },
                'vAxis': {
                    'title': 'Impedance'
                },
                'crosshair': {
                    'trigger': 'both', 'orientation': 'vertical'
                }, // Display crosshairs on focus and selection.
                'selectionMode': 'multiple'
            }

          });

          // Establish dependencies, declaring that 'filter' drives 'pieChart',
          // so that the pie chart will only display entries that are let through
          // given the chosen slider range.
          dashboard.bind(impedanceRangeSlider, impedanceChart);

          // Draw the dashboard.
          dashboard.draw(data);
        }

    }

})();
