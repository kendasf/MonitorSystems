var first_update = true;
var selected_panels_config = 0;
var paper = null;
var image_width = 0;
var image_height = 0;
var selected_font = 0;
var selected_mode = 0;
var crnt_config_mode = 0;
var blink_downcount = 0;
var crnt_frame_downcount = 0;
var crnt_frame = 0;
var sequence_bitmap_data = [];
var manual_display_brightness = 0;

var Speed30_0 = 
[
    1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0,
    0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1,
    0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1,
    0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1,
    0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1,
    1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1,
    0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0,
];


var Speed30_1 = 
[
    0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
    0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
];

var Speed30_2 = 
[
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 0,1, 1, 0, 0, 0, 0, 1, 1,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1,
    1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1,
    1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1,
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
];

var Speed30_3 = 
[
    1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,
    1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,0,
    0,0,0,0,0,0,1,1,1,0,0,1,1,1,0,0,0,0,1,1,1,
    0,0,0,0,0,1,1,1,0,0,0,1,1,0,0,0,0,0,0,1,1,
    0,0,0,0,1,1,1,0,0,0,0,1,1,0,0,0,0,0,1,1,1,
    0,0,0,0,1,1,1,0,0,0,0,1,1,0,0,0,0,1,1,1,1,
    0,0,0,0,0,1,1,1,0,0,0,1,1,0,0,0,1,1,0,1,1,
    0,0,0,0,0,0,1,1,1,0,0,1,1,0,0,1,1,0,0,1,1,
    0,0,0,0,0,0,0,1,1,1,0,1,1,0,1,1,0,0,0,1,1,
    0,0,0,0,0,0,0,0,1,1,0,1,1,1,1,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,1,1,0,1,1,1,0,0,0,0,0,1,1,
    1,1,1,0,0,0,0,1,1,1,0,1,1,1,0,0,0,0,0,1,1,
    0,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,0,
    0,0,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,0,0,
];

var Speed30_4 = [
    1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,
    0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,1,0,0,0,0,0,0,1,1,1,
    0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,1,0,0,0,0,0,0,1,1,1,
    0,0,0,0,0,0,0,1,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,1,1,1,
    0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,
    0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,0,0,0,0,0,1,1,0,1,1,
    0,0,0,0,0,0,1,1,1,0,0,0,0,1,1,0,0,0,0,1,1,0,0,1,1,
    0,0,0,0,0,0,0,1,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,
    0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,0,1,1,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,1,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,1,1,
    1,1,1,0,0,0,0,0,0,1,1,1,0,1,1,1,0,0,0,0,0,0,1,1,1,
    0,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,0,
    0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,
];

var Speed30_5 = [ 
    1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,1,1,0,1,1,1,0,0,1,1,1,
    0,0,0,0,0,1,1,1,0,1,1,0,0,0,0,1,1,
    0,0,0,0,1,1,1,0,0,1,1,0,0,0,1,1,1,
    0,0,0,1,1,1,0,0,0,1,1,0,0,1,1,1,1,
    0,0,0,0,1,1,1,0,0,1,1,0,1,1,0,1,1,
    0,0,0,0,0,1,1,1,0,1,1,1,1,0,0,1,1,
    0,0,0,0,0,0,1,1,0,1,1,1,0,0,0,1,1,
    1,1,0,0,0,0,1,1,0,1,1,0,0,0,0,1,1,
    1,1,1,0,0,1,1,1,0,1,1,1,0,0,1,1,1,
    1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,
    0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,
];

var Speed30_6 = [ 
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,1,1,1,
    0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,1,1,
    0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,1,0,1,1,
    0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,1,
    0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,1,1,
    0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,1,1,
    0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,
    0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0
];

function update_status() {
  var token = get_token();
  $.ajax("/get_status?token=" + token + "&ts=" + (+ new Date()), {
    dataType: "json" 
  }).done(function(response) {
    if (current_user_level == -1)
      return;

    if (response) {
      if (first_update) {
        if (current_user_level == 0) {
          create_select_option(['mph', 'km/h'], 'speed-units', response['speed-units']);
          create_select_option(["Spec. #28", "NMEA", "ASCII64", "No Radar"], 'radar-protocol', response['radar-protocol']);
          create_select_option(['9600', '38400', '57600', '115200'], 'radar-baud', response['radar-baud']);
          create_select_option(['0', '1', '2', '3', '4', '5', '6', '7'], 'radar-sensitivity', response['radar-sensitivity']);
        }
        else {
          $("#holder-param-speed-units").text(response['speed-units']);
          $("#holder-param-radar-protocol").text(response['radar-protocol']);
          $("#holder-param-radar-baud").text(response['radar-baud']);
          $("#holder-param-radar-sensitivity").text(response['radar-sensitivity']);
        }

        if (current_user_level != 2) {
          //$("#ip-address").val(response["ip-address"]);
          $("#wifi-name").val(response["wifi-name"]);

          $("#min-speed").val(response["min-speed"]);
          $("#max-speed").val(response["max-speed"]);

          $("#blink-on").val(response["blink-on"]);
          $("#blink-off").val(response["blink-off"]);

        }
        else {
          //$("#input-ip-address").html(response["ip-address"]);
          //$("#ip-address-btn").hide();

          $("#input-wifi-name").html(response["wifi-name"]);
          $("#wifi-name-btn").hide();

          $("#input-max-speed").html(response["max-speed"]);
          $("#max-speed-btn").hide();

          $("#input-min-speed").html(response["min-speed"]);
          $("#min-speed-btn").hide();

          $("#input-blink-on").html(response["blink-on"]);
          $("#blink-on-btn").hide();

          $("#input-blink-off").html(response["blink-off"]);
          $("#blink-off-btn").hide();
        }

        if (current_user_level == 2) {
          $("#dropdown-config-modes").hide();
        }

        if (current_user_level != 0) {
          $("#edit-dimming-table-cell").html('');
          $("#change-panels-config-row").html('');
        }
        
        $("#speed-limit").val(response["speed-limit"]);
        

        if (response["display-brightness"] & 0x80) {
          $("#autodimming-enabled").prop('checked', true);
          $("#display-brightness").prop('disabled', true);
          $("#autodimming-enabled-text").text("Enabled");
          $("#autodimming-enabled-btn").addClass("active");
          $("#dimming-table-btn").prop('disabled', false);
          $("#display-brightness").val(response["actual-brightness"] & 0x7F);
        }
        else {
          $("#display-brightness").val(response["display-brightness"] & 0x7F);
        }
      }

      if (first_update) {
        if ((response["dot_width"] % 8) == 0)
          var mode = 0;
        else
          var mode = 1;

        var row = response["dot_height"] / ((mode == 0) ? 10 : 8);
        var col = response["dot_width"] / ((mode == 1) ? 10 : 8); 

        var s = 0;
        var sets = [
          [1, 2, 0, 1],
          [2, 2, 0, 5],
          [2, 3, 0, 11],
          [2, 4, 0, 13],
          [3, 2, 0, 8],
          [3, 3, 0, 15],
          [3, 4, 0, 17],
          [2, 2, 1, 4],
          [2, 3, 1, 10],
          [2, 4, 1, 14],
          [3, 2, 1, 9],
          [3, 3, 1, 16],
          [3, 4, 1, 18],
          [3, 5, 1, 19],
          [4, 4, 1, 21],
          [4, 5, 1, 20]
        ];

        for (var i = 0; i < sets.length; i++) {
          if ((sets[i][0] == row) && (sets[i][1] == col) && (sets[i][2] == mode)) {
            s = sets[i][3];
            break;
          }
        }

        select_panels_config(row, col, mode, s);
      }
      first_update = false;

      var ro_params = ['system-time', 'firmware', 'supply-voltage', 'luminance'];
      for (p in ro_params) {
        $("#param-" + ro_params[p]).text(response[ro_params[p]]);
      }

      $("#dot-size").text(response["dot_width"] + " x " + response["dot_height"]);



      if (response["display-brightness"] & 0x80) {
        $("#display-brightness").val(response["actual-brightness"] & 0x7F);
      }

      manual_display_brightness = response["display-brightness"] & 0x7F;

    }     
  });
}

function create_select_option(options, param_name, value) {
  var values_html = '<div class="input-group" style="width: 100%">';
  values_html += '<div class="dropdown" style="width: 100%"><button class="btn btn-xs btn-default dropdown-toggle" type="button" id="dropdown-menu-' + param_name + '" data-toggle="dropdown" aria-haspopup="true" aria-expanded="true" style="width: 100%; text-align: left">';
  values_html += '<span class="caret pull-right" style="margin-top: 7px"></span>';
  values_html += '<span id="param-' + param_name + '">' + value + '</span>';
  
  values_html += '</button><ul class="dropdown-menu" aria-labelledby="dropdown-menu-' + param_name + '" id="dropdown-select-dlg-items" style="width: 100%">';
                        
  for (var o in options) {
    values_html += '<li><a href="#" onclick="change_select_value(\'' + param_name + '\', \'' + options[o] + '\')">' + options[o] + '</a></li>';
  }

  values_html += '</ul></div></div>';

  $("#holder-param-" + param_name).html(values_html);
}

function change_select_value(p, v) {
  $("#param-" + p).text(v);
  var token = get_token();


  $.ajax("/change_param?token=" + token + "&param=" + p + "&value=" + encodeURIComponent(v) + "&ts=" + (+new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    var cl = "danger";
    if (response && response.status == "ok")
      cl = "success";     

    $("#dropdown-menu-" + p).addClass("btn-" + cl, 400, 'linear');
    setTimeout(function() {
      $("#dropdown-menu-" + p).removeClass("btn-" + cl, 300);
    }, 400);
  });
}

function sync_time() {
  var token = get_token();

  var time = new Date();
  var timestr = "";
  timestr += (time.getYear() + 1900);
  timestr += format_two_digit(time.getMonth() + 1);
  timestr += format_two_digit(time.getDate());
  timestr += format_two_digit(time.getHours());
  timestr += format_two_digit(time.getMinutes());
  timestr += format_two_digit(time.getSeconds());

  $.ajax("/change_param?token=" + token + "&param=system-time&value=" + timestr + "&ts=" + (+ new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    var cl = "danger";
    if (response && response.status == "ok")
      cl = "success";     

    $("#sync-time-btn").blur();
    $("#sync-time-btn").addClass("btn-" + cl, 400);
    setTimeout(function() {
      $("#sync-time-btn").removeClass("btn-" + cl, 300);
    }, 400); 
  });
}

function on_text_update(p) {
  $("#" + p + "-btn").prop("disabled", false);
  $("#" + p + "-btn").addClass("btn-danger", 400);
}

function change_text_value(p) {
  var token = get_token();

  var val = $("#" + p).val();
  
  if (p == 'ip-address') {
    if (/^(?!0)(?!.*\.$)((1?\d?\d|25[0-5]|2[0-4]\d)(\.|$)){4}$/.test(val)) {
      var tokens = val.split('.');
      val = parseInt(tokens[0]);
      val |= parseInt(tokens[1]) << 8;
      val |= parseInt(tokens[2]) << 16;
      val |= parseInt(tokens[3]) << 24;
    }
    else {
      var cl = "danger";
      $("#" + p).addClass("btn-" + cl, 400, 'linear');
      setTimeout(function() {
        $("#" + p).removeClass("btn-" + cl, 300);
      }, 400);
      return;
    }
  }

  $.ajax("/change_param?token=" + token + "&param=" + p + "&value=" + encodeURIComponent(val) + "&ts=" + (+new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    var cl = "danger";
    if (response && response.status == "ok") {
      cl = "success";     
      $("#" + p + "-btn").prop("disabled", true);
      $("#" + p + "-btn").removeClass("btn-danger", 400);
    }

    $("#" + p).addClass("btn-" + cl, 400, 'linear');
    setTimeout(function() {
      $("#" + p).removeClass("btn-" + cl, 300);
    }, 400);
  });
}

function change_auto_dimming() {
  var autodimming = 0;
  if ($("#autodimming-enabled").prop('checked')) {
    $("#display-brightness").prop('disabled', true);
    $("#autodimming-enabled-text").text("Enabled");
    $("#dimming-table-btn").prop('disabled', false);
    autodimming = 1;
  }
  else {
    $("#display-brightness").prop('disabled', false);
    $("#autodimming-enabled-text").text("Disabled");
    $("#dimming-table-btn").prop('disabled', true);
    $("#display-brightness").val(manual_display_brightness);
  }

  var token = get_token();

  $.ajax("/change_param?token=" + token + "&param=autodimming&value=" + encodeURIComponent(autodimming) + "&ts=" + (+new Date())  , {
    dataType: "json" 
  }).done(function(response) {
  });
}

function edit_dimming_table() {
  var token = get_token();

  $.ajax("/get_dimming_table?token=" + token + "&ts=" + (+new Date()), {
    dataType: "json" 
  }).done(function(response) {
    if (response && response.table) {
      for (var i = 0; i < 16; i++) {
        $("#edit-intensity-" + i).val(response.table[i][0]);
        $("#edit-luminance-" + i).val(response.table[i][1]);
      }

      $("#auto-dimming-table").modal('show');
    }  
  });
}

function init_dimming_table() {
  var table_rows = "";
  for (var i = 0; i < 16; i++) {
    table_rows += "<tr>";
    table_rows += "<td>";
    table_rows += '<input type="number" min="0" max="100" style="width: 90%" id="edit-intensity-' + i + '"></input>';
    table_rows += "</td><td>";
    table_rows += '<input type="number" min="0" max="20000" style="width: 90%" id="edit-luminance-' + i + '"></input>';
    table_rows += "</td>";
    table_rows += "</tr>";
  }
  $("#dimming-table").html($("#dimming-table").html() + table_rows);

  if (current_user_level == 0) {
    var html = '<button type="button" class="btn btn-default btn-sm" aria-label="Left Align" id="upload-library" onclick="upload_library()">';
    html += '<span class="glyphicon glyphicon-upload" aria-hidden="true"></span> Upload image library</button>';
    $("#upload-library-placeholder").html(html);

    html = '<button type="button" class="btn btn-default btn-sm" aria-label="Left Align" id="upload-library" onclick="autoconf_radar()">';
    html += '<span class="glyphicon glyphicon-cog" aria-hidden="true"></span> Config radar</button>';
    $("#autoconf-radar-placeholder").html(html);
  }
  
}

function upload_library() {
  $("#file-upload-body").html("");
  $("#file-upload-body").html('<div class="input-group" style="width: 100%"><input type="file" name="upload-file" id="upload-file" class="form-control"/></div>');

  init_file_upload();
  $("#upload-vil-dlg").modal('show');
}

function autoconf_radar() {
  var token = get_token();
  $.ajax("/radar_autoconf?token=" + token + "&ts=" + (+new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    if (response.status)
      show_message("Autoconfigure radar", response.status);
    else
      show_message("Autoconfigure radar", "Error while communicating with the radar device.");
  });
}


function save_dimming_table() {
  var token = get_token();

  var values = "";
  for(var i = 0; i < 16; i++) {
    values += "&i" + i + "=" + $("#edit-intensity-" + i).val();
    values += "&l" + i + "=" + $("#edit-luminance-" + i).val();
  }

  $.ajax("/change_param?token=" + token + "&value=0&param=dimming-table" + values + "&ts=" + (+new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    $("#auto-dimming-table").modal('hide');
  });
}

function run_test() {
  $("#test-mode").modal('show');
}

function on_testmode_change() {
  if ($("#radio-all-leds").prop('checked'))
    $("#test-image-id").prop('disabled', true);
  else
    $("#test-image-id").prop('disabled', false);
}

function start_test_mode() {
  var token = get_token();

  var image_id = 0;
  if ($("#radio-all-leds").prop('checked') == false)
    image_id = $("#test-image-id").val();

  var duration = $("#test-duration").val();

  $.ajax("/start_test_mode?token=" + token + "&image_id=" + image_id + "&duration=" + encodeURIComponent(duration) + "&ts=" + (+new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    $("#test-mode").modal('hide');
  });
}

function change_panels_config() {
  $("#panels-config-dlg").modal('show');
}

function select_panels_config(row, col, mode, pc) {
  var txt = "";
  if (mode == 0)
    txt = "Portrait: ";
  else
    txt = "Landscape: ";

  if (row == 1)
    txt += "1 Row, ";
  else
    txt += row + " Rows, ";

  txt += col + " Columns";

  selected_panels_config = pc;

  var w = col * ((mode == 0) ? 8 : 10);
  var h = row * ((mode == 0) ? 10 : 8);

  $("#panels-config-text").text(txt + " (" + w + "x" + h + ")");
}

function save_panels_configuration() {
  var token = get_token();

  $.ajax("/change_param?token=" + token + "&param=panels-config&value=" + encodeURIComponent(selected_panels_config) + "&ts=" + (+new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    $("#panels-config-dlg").modal('hide');
  });
}

function change_password() {
  $("#selected-user").text("Power user");
  $("#old-password").val('');
  $("#new-password").val('');
  $("#new-password-2").val('');

  if (current_user_level != 0)
    $("#item-power-user").hide();
  if (current_user_level == 1) {
    $("#item-user-2").hide();
    select_user(1);
  }
  if (current_user_level == 2) {
    $("#item-user-1").hide();
    select_user(2);
  }


  $("#change-password-dlg").modal('show');
}

function select_user(ul) {
  var desc = "Power user";
  if (ul == 1)
    desc = "User level 1";
  if (ul == 2)
    desc = "User level 2";

  $("#selected-user").text(desc);
}

function do_change_password() {
  var token = get_token();

  var ul = 0;
  if ($("#selected-user").text() == "User level 1")
    ul = 1;
  if ($("#selected-user").text() == "User level 2")
    ul = 2;

  if ($("#new-password").val() != $("#new-password-2").val()) {
    show_message("Change password", "The re-entered password does not match the new password.");
    return;
  }

  if ($("#new-password").val().length < 6) {
    show_message("Change password", "The new password is too short (must be at least 6 characters long).");
    return;
  }

  var old_password = md5($("#old-password").val());

  var new_password = "";
  for (var i = 0; i < $("#new-password").val().length; i++) {
    var code = $("#new-password").val().charCodeAt(i) ^ $("#old-password").val().charCodeAt(i % $("#old-password").val().length);
    var hexcode = code.toString(16);
    if (hexcode.length == 1)
      hexcode = "0" + hexcode;
    new_password += hexcode;
  }

  $.ajax("/change_param?token=" + token + "&param=pwd&ul=" + ul + "&op=" + old_password + "&value=" + new_password + "&ts=" + (+new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    if (response && response.error) {
      show_message("Change password", response.error);
      return;
    }
    $("#change-password-dlg").modal('hide');
  });
}

function show_message(title, text) {
  $("#message-title").text(title);
  $("#message-text").text(text);
  $("#message-dlg").modal('show');
}

function edit_config_modes(id) {
  switch(id) {
    case 0: $("#display-modes-dlg-title").text("Edit Display Mode - No Vehicles Detected"); break;
    case 1: $("#display-modes-dlg-title").text("Edit Display Mode - Vehicle Below Speed Limit"); break;
    case 2: $("#display-modes-dlg-title").text("Edit Display Mode - Vehicle Above Speed Limit"); break;
    case 3: $("#display-modes-dlg-title").text("Edit Display Mode - Vehicle ABove Max. Speed"); break;
  }

  crnt_config_mode = id;
  
  var token = get_token();
  $.ajax("/get_panels_config?token=" + token + "&id=" + id + "&ts=" + (+new Date()), {
    dataType: "json" 
  }).done(function(response) {
    change_speed_display_settings(response.speedDisplayMode);
    change_speed_display_font(response.font);

    $("#speed-display-pos-x").val(response.x);
    $("#speed-display-pos-y").val(response.y);

    $("#speed-display-frame-duration").val(response.frameLength);
    $("#speed-display-frames-count").val(response.numFrames);

    for (var i = 0; i < 10; i++) {
      $("#speed-display-frame-id-" + i).val(response.frames[i]);
      if (i < response.numFrames)
        $("#speed-display-frame-id-" + i).show();
      else
        $("#speed-display-frame-id-" + i).hide();
    }

    if (response.numFrames == 0)
      $("#bitmap-ids-title").hide();
    else
      $("#bitmap-ids-title").show();

    try{
      if (paper)
        paper.remove();
    }
    catch (error) {
        // this catches the error and allows you to proceed along nicely
    }

    change_sequence();

    image_width = response.imageWidth;
    image_height = response.imageHeight;

    paper = new Raphael(document.getElementById('image-editor'), image_width, image_height);
    paper.setViewBox(0, 0, image_width * 10, image_height * 10, true);
    paper.setSize('100%', '100%');

    $("#display-modes-dlg").modal('show');  
    refresh_preview();
  }); 
}

function change_speed_display_settings(id) {
  switch(id) {
    case 0: $("#speed-display-settings").text("Do not display speed"); break;
    case 1: $("#speed-display-settings").text("Display speed"); break;
    case 2: $("#speed-display-settings").text("Display speed with blinking"); break;
  }

  update_preview_timers();
  selected_mode = id;
}

function change_speed_display_font(id) {
  switch(id) {
    case 0: $("#speed-display-font").text('Small 7"'); break;
    case 1: $("#speed-display-font").text('Normal 9"'); break;
    case 2: $("#speed-display-font").text('Bold 10"'); break;
    case 3: $("#speed-display-font").text('Large 13"'); break;
    case 4: $("#speed-display-font").text('Huge 18"'); break;
    case 5: $("#speed-display-font").text('Enormous 24"'); break;
  }
  selected_font = id;
  refresh_preview();
}

function change_x_pos() {
  refresh_preview();
}

function change_y_pos() {
  refresh_preview();
}

function change_frame_duration() {
  update_preview_timers();
}

function change_number_of_frames() {
  var num_frames = parseInt($("#speed-display-frames-count").val());

  for (var i = 0; i < num_frames; i++) {
    $("#speed-display-frame-id-" + i).show();
  }
  for (var i = num_frames; i < 10; i++) {
    $("#speed-display-frame-id-" + i).hide();
  }

  if (num_frames > 10)
    $("#speed-display-frames-count").val('10')

  if (num_frames == 0)
    $("#bitmap-ids-title").hide();
  else
    $("#bitmap-ids-title").show();

  if (parseInt($("#speed-display-frame-duration").val()) == 0)
    $("#speed-display-frame-duration").val("850");

  update_preview_timers();
}

function change_sequence() {
  load_sequence_bitmap_data(0, []);
}

function load_sequence_bitmap_data(id, data) {
  var token = get_token();
   $.ajax("/get_bitmap_data?token=" + token + "&id=" + (parseInt($("#speed-display-frame-id-" + id).val()) - 1) + "&ts=" + (+new Date()), {
    dataType: "json" 
  }).done(function(response) {
    if (response) {
      data.push(response.image_data);
      if (id < 9)
        load_sequence_bitmap_data(id+1, data);
      else {
        sequence_bitmap_data = data;
        update_preview_timers();
      }
    }
  });
}

function update_preview_timers() {
  blink_downcount = 850 + 850;
  crnt_frame_downcount = parseInt($("#speed-display-frame-duration").val());
  crnt_frame = 0;
}

function update_preview() {
  var repaint = false;

  blink_downcount -= 50;
  if ((blink_downcount <= 850) && ((blink_downcount + 50) >= 850)) {
      repaint = true;
  }

  if (blink_downcount <= 0) {
      blink_downcount = 850 + 850;
      repaint = true;
  }

  crnt_frame_downcount -= 50;
  if (crnt_frame_downcount <= 0)
  {
      crnt_frame++;
      if (crnt_frame >= parseInt($("#speed-display-frames-count").val()))
          crnt_frame = 0;

      crnt_frame_downcount = parseInt($("#speed-display-frame-duration").val());

      repaint = true;
  }

  if (repaint)
    refresh_preview();
}

function refresh_preview() {
  if (paper == null)
    return;

  paper.clear();

  var bkg = paper.rect(0, 0, image_width * 10, image_height * 10);
  bkg.attr( { fill: '#000000'});

  if (selected_mode > 0) {
    if ((selected_mode == 1) || (blink_downcount > 850))
      display_numbers();
  }

  if (crnt_frame < parseInt($("#speed-display-frames-count").val()))
    display_image(crnt_frame);
}

function display_numbers() {
  var x = $("#speed-display-pos-x").val();
  var y = $("#speed-display-pos-y").val();

  x = parseInt(x);
  y = parseInt(y);

  if (selected_font == 0)
  {
      for (var i = 0; i < 7; i++)
      {
          for (var j = 0; j < 11; j++)
          {
              if (Speed30_0[i * 11 + j] != 0)
              {
                  var dot = paper.rect((x + j) * 10, (y + i) * 10, 10, 10);
                  dot.attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
              }
          }
      }
  }
  if (selected_font == 1)
  {
      for (var i = 0; i < 10; i++)
      {
          for (var j = 0; j < 16; j++)
          {
              if (Speed30_1[i * 16 + j] != 0)
              {
                  var dot = paper.rect((x + j) * 10, ( y + i) * 10, 10, 10);
                  dot.attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
              }
          }
      }
  }
  if (selected_font == 2)
  {
      for (var i = 0; i < 10; i++)
      {
          for (var j = 0; j < 17; j++)
          {
              if (Speed30_2[i * 17 + j] != 0)
              {
                  var dot = paper.rect((x + j) * 10, ( y + i) * 10, 10, 10);
                  dot.attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
              }
          }
      }
  }
  if (selected_font == 3)
  {
      for (var i = 0; i < 13; i++)
      {
          for (var j = 0; j < 17; j++)
          {
              if (Speed30_5[i * 17 + j] != 0)
              {
                  var dot = paper.rect((x + j) * 10, ( y + i) * 10, 10, 10);
                  dot.attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
              }
          }
      }
  }
  if (selected_font == 4)
  {
      for (var i = 0; i < 18; i++)
      {
          for (var j = 0; j < 25; j++)
          {
              if (Speed30_4[i * 25 + j] != 0)
              {
                  var dot = paper.rect((x + j) * 10, ( y + i) * 10, 10, 10);
                  dot.attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
              }
          }
      }
  }
  if (selected_font == 5)
  {
      for (var i = 0; i < 24; i++)
      {
          for (var j = 0; j < 29; j++)
          {
              if (Speed30_6[i * 29 + j] != 0)
              {
                  var dot = paper.rect((x + j) * 10, ( y + i) * 10, 10, 10);
                  dot.attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
              }
          }
      }
  }
}

function display_image(frame) {
  if ((sequence_bitmap_data.length < frame) || (!sequence_bitmap_data[frame]))
    return;

  for (var x = 0; x < image_width; x++) {
    for (var y = 0; y < image_height; y++) {
      if (sequence_bitmap_data[frame][y][x]) {
        var dot = paper.rect((x) * 10, (y) * 10, 10, 10);
        dot.attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
      }
    }
  }
}

function stop_preview_update() {
  try {
    if (paper)
      paper.remove();
  }
  catch (error) {
      // this catches the error and allows you to proceed along nicely
  }
  paper = null;
}

function save_display_mode() {
  var token = get_token();

  var speedDisplayMode = selected_mode;
  var x = $("#speed-display-pos-x").val();
  var y = $("#speed-display-pos-y").val();
  var font = selected_font;
  var frameLength = $("#speed-display-frame-duration").val();
  var numFrames = $("#speed-display-frames-count").val();

  var query = "&id=" + crnt_config_mode;
  query += "&speedDisplayMode=" + speedDisplayMode;
  query += "&x=" + x;
  query += "&y=" + y;
  query += "&font=" + font;
  query += "&frameLength=" + frameLength;
  query += "&numFrames=" + numFrames;
  
  for(var i = 0; i < 10; i++)
    query += "&frame-" + i + "=" + $("#speed-display-frame-id-" + i).val();

  $.ajax("/set_panels_config?token=" + token + query + "&ts=" + (+ new Date()), {
    dataType: "json" 
  }).done(function(response) {
    if (response) {
      stop_preview_update();
      $("#display-modes-dlg").modal('hide'); 
    }
  });
}

function init_file_upload() {
  $("#upload-file").fileinput({
    uploadUrl: '/upload?token=' + get_token(),
    showRemove: false,
    showClose: false,
    maxFileCount: 1,
    autoReplace: true,
    allowedFileExtensions: ['vil'],
    uploadAsync: false,
    dropZoneTitle: 'Drag & drop library file here...',
    msgUploadThreshold: 'Uploading...',
    uploadLabel: 'Upload!',
    uploadClass: 'btn btn-danger',
    layoutTemplates: {
      size: ' <br><small>({sizeText})</small>',
      footer: '<div class="file-thumbnail-footer"><div class="file-footer-captiona" title="{caption}"><strong>{caption}</strong>{size}<span id="fw-version"></span></div></div>',
      progress: '<div class="progress">\n' +
          '    <div class="progress-bar progress-bar-danger progress-bar-striped active text-center" role="progressbar" aria-valuenow="{percent}" aria-valuemin="0" aria-valuemax="100" style="width:{percent}%;">\n' +
          '        {percent}%\n' +
          '     </div>\n' +
          '</div>',
    },
    previewTemplates: {
      object: '<div class="file-preview-frame{frameClass}" id="{previewId}" data-fileindex="{fileindex}" data-template="{template}"' +
          ' title="{caption}">\n' +
          '   <div class="kv-file-content"><h2><span class="glyphicon glyphicon-file"></span></h2>' +
          '       <div class="kv-preview-data file-preview-other-frame">' +
          '       </div>\n' +
          '   </div>\n' +
          '   <div class="file-preview-other-footer">{footer}</div>\n' +
          '</div>'
    },
    fileActionSettings: {
      showUpload: false,
      showZoom: false
    }
  });

  $("#upload-file").on('filebatchuploadsuccess', function(event, data, previewId, index) {
    $("#upload-vil-dlg").modal('hide');

    if (data.response && data.response.status == 'ok')
      show_message("Upload Image Library", "Upload successfull.");
    else
      show_message("Upload Image Library", "Failed to upload the image library: " + data.response.status);
  });

  $("#upload-file").on('filebatchuploaderror', function(event, data, previewId, index) {
    $("#upload-vil-dlg").modal('hide');
    show_message("Upload Image Library", "Failed to upload the image library.");
  });

}


check_token();

setInterval(update_status, 200);

setTimeout(init_dimming_table, 200);

setInterval(update_preview, 50);

$(function () {
  $('[data-toggle="tooltip"]').tooltip()
})
