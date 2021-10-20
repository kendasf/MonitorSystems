var weekdays = [
   "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
];

var months = [
  "January", "February", "March", "April", "May", "June", 
  "July", "August", "September", "October", "November", "December"
];

var ordinals = [
   "1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th", "10th",
   "11th", "12th", "13th", "14th", "15th", "16th", "17th", "18th", "19th", "20th",
   "21st", "22nd", "23rd", "24th", "25th", "26th", "27th", "28th", "29th", "30th", "31st"
];

function initialize() {
  var token = get_token();
  $.ajax("/get_schedule?token=" + token + "&ts=" + (+ new Date()), {
    dataType: "json" 
  }).done(function(response) {
    if (response["schedule-type"] != 0) {
      $("#keep-sign-always-on-text").text("Turn the sign ON during defined time intervals");
      $("#autodimming-enabled").prop("checked", true);
      $("#keep-sign-always-on").addClass("active");
    }

    var rows = '<table class="table table-striped">';
    for (var i = 0; i < response.schedules.length; i++) {
      rows += create_row(response.schedules[i], i);
    }
    rows += "</table>";

    $("#scheduler-list").html(rows);
  });
}

function create_row(r, i) {
  var row = "<tr>"
  row += "<td>";

  if (r["entryType"] == 0)
    row += "Every day, ";
  else if (r["entryType"] == 1)
    row += "Every " + weekdays[r["dow"]] + ", ";
  else
    row += "On " + r["year"] + "-" + r["month"] + "-" + r["day"] + ", ";

  var mid = r["start"];
  var dur = r["duration"];
  row += "starts at " + format_two_digit((mid / 60) | 0) + ":" + format_two_digit(mid % 60);
  row += ", duration " + format_two_digit((dur / 60) | 0) + ":" + format_two_digit(dur % 60);

  var sday = r["sday"];
  var smonth = r["smonth"];
  var eday = r["eday"];
  var emonth = r["emonth"];

  if ((sday != 1) || (eday != 31) || (smonth != 1) || (emonth != 12)) {
    row += ", valid between " + months[smonth - 1] + " " + ordinals[sday - 1];
    row += " and " + months[emonth - 1] + " " + ordinals[eday - 1];
  }
  row += ".";
  row += "</td>";

  row += '<td><button type="button" class="btn btn-danger btn-xs" data-toggle="tooltip" data-placement="top" title="Delete" onclick="delete_entry(' + i+ ')"><span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button></td>';

  row += "</tr>";
  return row;
}

function delete_entry(i) {
  var token = get_token();

  $.ajax("/delete_schedule_entry?token=" + token + "&id=" + i + "&ts=" + (+ new Date()), {
    dataType: "json" 
  }).done(function(response) {
    initialize();
  });
}

function change_use_scheduler() {
  var active = 0;

  if ($("#autodimming-enabled").prop("checked")) {
    $("#keep-sign-always-on-text").text("Turn the sign ON only during defined time intervals");
    active = 1;
  }
  else
    $("#keep-sign-always-on-text").text("Keep sign always on - the defined intervals are ignored");

  var token = get_token();
  $.ajax("/activate_schedule?token=" + token + "&active=" + active + "&ts=" + (+ new Date()), {
    dataType: "json" 
  }).done(function(response) {

  });
}

function add_interval() {
  $("#add-inteval-dlg").modal('show');
}

function set_event_active(id) {
  $("#exact-date").hide();
  switch (id) {
    case 0: $("#event-active").text("Every day"); break;
    case 1: $("#event-active").text("Sunday"); break;
    case 2: $("#event-active").text("Monday"); break;
    case 3: $("#event-active").text("Tuesday"); break;
    case 4: $("#event-active").text("Wednesday"); break;
    case 5: $("#event-active").text("Thursday"); break;
    case 6: $("#event-active").text("Friday"); break;
    case 7: $("#event-active").text("Saturday"); break;
    case 8: $("#event-active").text("Set exact date"); $("#exact-date").show(); break;
  }
}

function do_add_interval() {
  var token = get_token();

  var entryType = 1;
  if ($("#event-active").text() == "Set exact date")
    entryType = 2;
  if ($("#event-active").text() == "Every day")
    entryType = 0;

  var dow = 0;
  for(var i = 0; i < 7; i++) {
    if (weekdays[i] == $("#event-active").text()) {
      dow = i;
      break;
    }
  }

  var year = $("#exact-date").datepicker('getDate').getYear() + 1900;
  var month = $("#exact-date").datepicker('getDate').getMonth() + 1;
  var day = $("#exact-date").datepicker('getDate').getDate();

  var start = $("#start-time").val().split(":");
  start = parseInt(start[0]) * 60 + parseInt(start[1]);

  var duration = $("#interval-duration").val().split(":");
  duration = parseInt(duration[0]) * 60 + parseInt(duration[1]);

  var sday = $("#start-date").datepicker('getDate').getDate();
  var smonth = $("#start-date").datepicker('getDate').getMonth() + 1;
  var eday = $("#end-date").datepicker('getDate').getDate();
  var emonth = $("#end-date").datepicker('getDate').getMonth() + 1;

  $.ajax("/add_schedule?token=" + token + "&entryType=" + entryType + "&dow=" + dow + "&year=" + year + "&month=" + month + "&day=" + day +
    "&sday=" + sday + "&smonth=" + smonth + "&eday=" + eday + "&emonth=" + emonth + "&start=" + start + "&duration=" + duration + "&ts=" + (+ new Date())
    , {
    dataType: "json" 
  }).done(function(response) {
    initialize();
    $("#add-inteval-dlg").modal('hide');
  });
}

function update_status() {
  var token = get_token();
  $.ajax("/get_status?token=" + token + "&ts=" + (+ new Date()), {
    dataType: "json" 
  }).done(function(response) {
    if (response) {
    }
  });
}

$("#exact-date").datepicker({
  format: 'mm/dd/yyyy'
});
$("#exact-date").datepicker('update', new Date());
$("#exact-date").hide();

$("#start-date").datepicker({
  format: 'mm/dd'
});
$("#start-date").datepicker('update', '01/01');

$("#end-date").datepicker({
  format: 'mm/dd'
});
$("#end-date").datepicker('update', '12/31');

$('#start-time').clockpicker({autoclose: true});
$('#interval-duration').clockpicker({autoclose: true});



check_token(1);
setTimeout(initialize, 100);
setInterval(update_status, 2000);