var crnt_folder = "";
var file_to_delete = "";

function refresh_files_list(files) {
  var html = '<table class="table table-striped">';
  html += "<tr><th width=\"25px\"></th><th>Filename</th><th>Size</th><th>Date</th><th>Actions</th></tr>";

  if (crnt_folder != "")
    html += create_folder_entry({name: "..", dir: true, date: ""});

  var files_html = "";
  var folders_html = "";

  for (var i = 0; i < files.length; i++) {
    if (files[i].dir)
      folders_html += create_folder_entry(files[i]);
    else
      files_html += create_file_entry(files[i]);
  }

  html += folders_html;
  html += files_html;

  html += '</table>';
  $("#files-list").html(html);

  $(function () {
    $('[data-toggle="tooltip"]').tooltip()
  })
}

function show_message(title, text) {
  $("#message-title").text(title);
  $("#message-text").text(text);
  $("#message-dlg").modal('show');
}

function create_file_entry(file_desc) {
  var html = "";
  var icon = "glyphicon-file";
  if (file_desc.name.substr(-3) == "txt")
    icon = "glyphicon-font";
  if (file_desc.name.substr(-3) == "bvs")
    icon = "glyphicon-list-alt";
  if (file_desc.name.substr(-3) == "dat")
    icon = "glyphicon-cog";
  if (file_desc.name.substr(-3) == "jpg")
    icon = "glyphicon-picture";
  if (file_desc.name.substr(-3) == "png")
    icon = "glyphicon-picture";
  if (crnt_folder == "/Photo")
    icon = "glyphicon-film";
  html += "<tr><td><span class=\"glyphicon " + icon + '" aria-hidden="true"></span></td><td>' + file_desc.name + "</td><td>" + format_size(file_desc.size) + "</td><td>" + file_desc.date + "</td>";
  html += '<td>';
  html += '<a type="button" class="btn btn-default btn-xs" data-toggle="tooltip" data-placement="top" title="Download" href="' + crnt_folder + '/' + file_desc.name + '"><span class="glyphicon glyphicon-download-alt" aria-hidden="true"></span></a>';
  if (file_desc.name.substr(-3) == "bvs") {
      html += '&nbsp;<button type="button" class="btn btn-success btn-xs" data-toggle="tooltip" data-placement="top" title="Generate PDF Report" onclick="make_pdf_report(\'' + file_desc.name + '\')"><span class="glyphicon glyphicon-stats" aria-hidden="true"></span></button>';
  }
  html += '&nbsp;<button type="button" class="btn btn-danger btn-xs" data-toggle="tooltip" data-placement="top" title="Delete" onclick="delete_file(\'' + file_desc.name + '\')"><span class="glyphicon glyphicon-trash" aria-hidden="true"></span></button>';
  html += '</td>';
  html += "</tr>";
  return html;
}

function create_folder_entry(file_desc) {
  var html = "";
  var icon = file_desc.name == '..' ? "glyphicon-arrow-left" : "glyphicon-folder-close";
  html += '<tr><td><a href="#" onclick="enter_folder(\'' + file_desc.name + '\')"><span class="glyphicon ' + icon + '" aria-hidden="true"></span></a></td><td>';
  html += '<a href="#" onclick="enter_folder(\'' + file_desc.name + '\')">' + file_desc.name + "</a></td><td></td><td>" + file_desc.date + "</td><td></td></tr>";
  return html;
}

function update_files_list() {
  var token = get_token();
  $.ajax("/list_folder?token=" + token + "&path=" + encodeURIComponent(crnt_folder) + "&ts=" + (+new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    if (response.status == "ok")
      refresh_files_list(response.files);
  });
}

function enter_folder(folder_name) {
  if (folder_name != "..")
    crnt_folder = crnt_folder + "/" + folder_name;
  else
     crnt_folder = crnt_folder.split('/').slice(0, crnt_folder.split('/').length - 1).join('/');

  $("#current-folder").text(crnt_folder);
  update_files_list();
}

function delete_file(file) {
  $("#delete-file-name").text(file);
  $("#confirm-delete-dialog").modal('show');
  file_to_delete = crnt_folder + "/" + file;
}

function perform_delete() {
  $('#confirm-delete-dialog').modal('hide');
  var token = get_token();
  $.ajax("/delete_file?token=" + token + "&path=" + encodeURIComponent(file_to_delete) + "&ts=" + (+ new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    update_files_list();
  });
}

function make_pdf_report(file) { 
  var token = get_token();

  $.ajax("/get_bvs_data?token=" + token + "&path=" + encodeURIComponent(crnt_folder + "/" + file) + "&ts=" + (+ new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    if (!response.values) {
      if (response.status)
        show_message("Generate PDF Report", "There was an error generating PDF report ("  + response.status + ")");
      else
        show_message("Generate PDF Report", "There was an error generating PDF report.");
      return;
    }


    $("#report-title").val("");
    $("#report-author").val("");
    $("#report-location").val("");
    $("#report-comments").val("");

    $("#pdf-report-dlg").modal("show");

    global_response = response;

  });
}

function start_pdf_generate() {
  $("#pdf-report-dlg").modal("hide");

  try {
    create_pdf_report(global_response.values, global_response.speed_limit);
  }
  catch (ex) {
    show_message('Generate PDF Report', 'Date/time values in the file are corrupt. Please download the BVS file and use off-line viewer to view this file.');
  }
}

function create_pdf_report(data, speed_limit) {
  var date_string = "";
  var crnt_time = new Date();
  date_string += (crnt_time.getMonth() + 1) + "/" + crnt_time.getDate() + "/" + (1900 + crnt_time.getYear());

  var pdf_options = {
    font: "helvetica",
    title: $("#report-title").val(),
    date_from: "12/1/2016",
    date_to: "12/6/2016",
    report_created: date_string, 
    author: $("#report-author").val(),
    location: $("#report-location").val(),
    file: "report.pdf",
    comments: $("#report-comments").val(),
    created_with: "Created with Monitor Systems Control Center",
    summary: true,
    speed_histogram: true,
    speed_limit_compliance: true,
    compliance_by_hour: true,
    violations_by_hour: true,
    vehicles_by_speed_hour: true,
    vehicles_by_hour_day: true,
    violations_by_hour_day: true,
    vehicles_by_hour_weekday: true,
    direction: "Both",
    violations_count: 0,
    speed_limit: speed_limit + " mph",
    count_to: "250",
    speed_histogram_color: [0x5d, 0xa5, 0xda], 
    comp_by_hour_count_to: 10,
    comp_by_hour_color: {
          at_or_below_speed_limit : [0x60, 0xBD, 0x68],
          at_or_below_speed_limit_plus5kmh : [0xDE, 0xCF, 0x3F],
          at_or_below_speed_limit_plus10kmh : [0xFA, 0xA4, 0x3A],
          above_speed_limit : [0xF1, 0x58, 0x54],
          },
    violations_by_hour_color: [0xF1, 0x58, 0x54],
    hours: ["00:00 - 00:59","01:00 - 01:59", "02:00 - 02:59", "03:00 - 03:59", "04:00 - 04:59", "05:00 - 05:59", "06:00 - 06:59", "07:00 - 07:59", "08:00 - 08:59", "09:00 - 09:59", "10:00 - 10:59", "11:00 - 11:59", "12:00 - 12:59", "13:00 - 13:59", "14:00 - 14:59", "15:00 - 15:59", "16:00 - 16:59", "17:00 - 17:59", "18:00 - 18:59", "19:00 - 19:59", "20:00 - 20:59", "21:00 - 21:59", "22:00 - 22:59", "23:00 - 23:59"],
    speed_intervals: ["0 - 10", "11 - 15", "16 - 20", "21 - 25", "26 - 30", "31 - 35", "36 - 40", "41 - 45", "46 - 50", "51 - 55", "56 - 60", "61 - 65", "66 - 70", "71 +"],
    weekdays: ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"],
    description_vehicles_by_speed_hour: "",
  };

  var speed_sum = 0;
  var speeds = [];
  var speed_hist = [];
  for(var i = 0; i < 250; i++)
    speed_hist.push(0);

  var count_below_limit = 0;
  var count_below_limit_5 = 0;
  var count_below_limit_10 = 0;
  var count_above_limit_10 = 0;

  var compliance_by_hour = [];
  var vehicles_by_speed_hour_data = [];
  var vehicles_by_hour_day_data = [];

  var violations_by_hour_day_data = [];

  var days = [];

  var vehicles_by_hour_weekday_data = [];
  for (var i = 0; i < 24; i++) {
    vehicles_by_hour_weekday_data.push([]);
    for(var j = 0; j < 7; j++) {
      vehicles_by_hour_weekday_data[i].push(0);
    }
  }

  var prev_day = -1;
  for (var i = 0; i < data.length / 7; i++) {
    var year = data[i * 7] + 2000;
    var month = data[i * 7 + 1];
    var day = data[i * 7 + 2];
    var hour = data[i * 7 + 3];
    var minute = data[i * 7 + 4];
    var second = data[i * 7 + 5];
    var speed = data[i * 7 + 6];

    var timestamp = new Date(year, month - 1, day, hour, minute, second);
    var day_str = pdf_options.weekdays[timestamp.getDay()] + " " + month + "/" + ((day >= 10) ? day : ("0" + day));

    if (prev_day == -1)
      prev_day = day;

    if ((days.length == 0) || (days[days.length - 1] != day_str)) {
      prev_day ++;
      while (day > prev_day) {
        var extra_timestamp = new Date(year, month - 1, prev_day, hour, minute, second);
        var extra_day_str = pdf_options.weekdays[extra_timestamp.getDay()] + " " + month + "/" + ((prev_day >= 10) ? prev_day : ("0" + prev_day));
        days.push(extra_day_str);
        prev_day ++;
      }
      days.push(day_str);
    }

    prev_day = day;

    if (!compliance_by_hour[hour])
      compliance_by_hour[hour] = {hour: "" + hour, ok: 0, _5: 0, _10: 0, above: 0};

    if (!vehicles_by_speed_hour_data[hour])
      vehicles_by_speed_hour_data[hour] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

    if (!vehicles_by_hour_day_data[hour]) {
      vehicles_by_hour_day_data[hour] = [];
      violations_by_hour_day_data[hour] = [];
    }

    if (!vehicles_by_hour_day_data[hour][days.length - 1])
      vehicles_by_hour_day_data[hour][days.length - 1] = 1;
    else
      vehicles_by_hour_day_data[hour][days.length - 1] ++;

    if (speed > speed_limit) {
      if (!violations_by_hour_day_data[hour][days.length - 1])
        violations_by_hour_day_data[hour][days.length - 1] = 1;
      else
        violations_by_hour_day_data[hour][days.length - 1] ++;
    }

    vehicles_by_hour_weekday_data[hour][timestamp.getDay()] ++;

    speed_sum += speed;
    speeds.push(speed);
    speed_hist[speed] ++;

    var cat = 0;
    if (speed >= 71)
      cat = 13;
    else if (speed >= 11)
      cat = (((speed - 11) / 5) | 0) + 1;

    vehicles_by_speed_hour_data[hour][cat] ++;

    if (speed <= speed_limit) {
      count_below_limit ++;
      compliance_by_hour[hour].ok ++;
    }
    else if (speed <= speed_limit + 5) {
      count_below_limit_5 ++;
      compliance_by_hour[hour]._5 ++;
    }
    else if (speed <= speed_limit + 10) {
      count_below_limit_10 ++;
      compliance_by_hour[hour]._10 ++;
    }
    else {
      count_above_limit_10 ++;
      compliance_by_hour[hour].above ++;
    }

    if (i == 0)
      pdf_options.date_from = month + "/" + day + "/" + year;

    pdf_options.date_to = month + "/" + day + "/" + year;

    if (speed >= speed_limit)
      pdf_options.violations_count ++;
  }

  for (var i = 0; i < 24; i++) {
    if (!vehicles_by_hour_day_data[i]) {
      vehicles_by_hour_day_data[i] = [];
      for (var j = 0; j < days.length; j++) {
        vehicles_by_hour_day_data[i].push(0);
      }
      continue;
    }

    for (var j = 0; j < days.length; j++) {
      if (!vehicles_by_hour_day_data[i][j])
        vehicles_by_hour_day_data[i][j] = 0;

      if (!violations_by_hour_day_data[i][j])
        violations_by_hour_day_data[i][j] = 0;
    }
  }

  pdf_options.total_vehicles = data.length / 7;
  if (pdf_options.total_vehicles > 0)
    pdf_options.average_speed = (speed_sum / pdf_options.total_vehicles).toFixed(2) + " mph";
  else
    pdf_options.average_speed = "0 mph";

  if (pdf_options.total_vehicles > 0)
    pdf_options.violations_count = pdf_options.violations_count + " (" + ((100 * pdf_options.violations_count / pdf_options.total_vehicles) | 0) + "%)";

  speeds.sort();

  pdf_options._85th_percentile = speeds[(speeds.length * 0.85) | 0] + " mph";
  pdf_options._70th_percentile = speeds[(speeds.length * 0.70) | 0] + " mph";
  pdf_options._50th_percentile = speeds[(speeds.length * 0.50) | 0] + " mph";

  var max_pace = 0;
  for (var i = 0; i < 190; i++) {
      var cPace = 0;
      for (var j = 0; j < 10; j++)
          cPace += speed_hist[i + j];

      if (cPace >= max_pace) {
          max_pace = cPace;
          pdf_options.pace_10kmh = i + " mph";
      }
  }

  pdf_options.speed_histo_data = [];
  for (var i = 10; i <= 90; i+= 5) {
    var sum_vehicles = 0;
    var last = 0;
    if (i > 10)
      last = i - 4;
    for (var j = i; j >= last; j--)
      sum_vehicles += speed_hist[j];

    pdf_options.speed_histo_data.push({
      span: last + "-" + i,
      value: sum_vehicles
    });
  }

  pdf_options.speed_limit_compliance_data = [
    {
      name: "At or below speed limit",
      percentage : pdf_options.total_vehicles > 0 ? ((100 * count_below_limit / pdf_options.total_vehicles) | 0) : 0,
      color : [0x60, 0xBD, 0x68]
    },
    {
      name : "At or below speed limit + 5 mph",
      percentage : pdf_options.total_vehicles > 0 ? ((100 * count_below_limit_5 / pdf_options.total_vehicles) | 0) : 0,
      color : [0xDE, 0xCF, 0x3F]
    },
    {
      name : "At or below speed limit + 10 mph",
      percentage : pdf_options.total_vehicles > 0 ? ((100 * count_below_limit_10 / pdf_options.total_vehicles) | 0) : 0,
      color : [0xFA, 0xA4, 0x3A]
    },
    {
      name : "Above speed limit + 10 mph",
      percentage : pdf_options.total_vehicles > 0 ? ((100 * count_above_limit_10 / pdf_options.total_vehicles) | 0) : 0,
      color : [0xF1, 0x58, 0x54]
    }
  ];

  var sum = 0;
  for (var i = 0; i < 4; i++)
    sum += pdf_options.speed_limit_compliance_data[i].percentage;

  var p = 0;
  while (sum < 100) {
    pdf_options.speed_limit_compliance_data[p++].percentage ++;
    sum ++;
    if (p == 4)
      p = 0;
  }

  pdf_options.total_vehicles = "" + pdf_options.total_vehicles;

  for (var i in compliance_by_hour) {
    var sum = compliance_by_hour[i].ok + compliance_by_hour[i]._5 + compliance_by_hour[i]._10 + compliance_by_hour[i].above;
    if (sum > pdf_options.comp_by_hour_count_to)
      pdf_options.comp_by_hour_count_to = sum;
  }

  if (pdf_options.comp_by_hour_count_to <= 10)
    pdf_options.comp_by_hour_count_to = 10;
  else if (pdf_options.comp_by_hour_count_to <= 50)
    pdf_options.comp_by_hour_count_to = 50;
  else if (pdf_options.comp_by_hour_count_to <= 100)
    pdf_options.comp_by_hour_count_to = 100;
  else if (pdf_options.comp_by_hour_count_to <= 200)
    pdf_options.comp_by_hour_count_to = 200;
  else if (pdf_options.comp_by_hour_count_to <= 500)
    pdf_options.comp_by_hour_count_to = 500;
  else if (pdf_options.comp_by_hour_count_to <= 1000)
    pdf_options.comp_by_hour_count_to = 1000;
  else if (pdf_options.comp_by_hour_count_to <= 2000)
    pdf_options.comp_by_hour_count_to = 2000;
  else if (pdf_options.comp_by_hour_count_to <= 5000)
    pdf_options.comp_by_hour_count_to = 5000;
  else if (pdf_options.comp_by_hour_count_to <= 10000)
    pdf_options.comp_by_hour_count_to = 10000;

  pdf_options.compliance_by_hour_data = compliance_by_hour;

  pdf_options.vehicles_by_speed_hour_data = vehicles_by_speed_hour_data;

  pdf_options.vehicles_by_speed_hour_percentage_total = calc_perc_total(vehicles_by_speed_hour_data);

  pdf_options.days = days;

  pdf_options.vehicles_by_hour_day_data = vehicles_by_hour_day_data;
  pdf_options.vehicles_by_hour_day_percentage_total = calc_perc_total(vehicles_by_hour_day_data);

  pdf_options.vehicles_by_hour_weekday_data = vehicles_by_hour_weekday_data;
  pdf_options.vehicles_by_hour_weekday_percentage_total = calc_perc_total(vehicles_by_hour_weekday_data);

  pdf_options.violations_by_hour_day_data = violations_by_hour_day_data;
  pdf_options.violations_by_hour_day_percentage_total = calc_perc_total(violations_by_hour_day_data);

  generate_pdf(pdf_options);  
}

function calc_perc_total(data) {
  var perc_total = [[], []];

  var cols = 0;
  for (var i in data) {
    cols = data[i].length;
    break;
  }

  var total_total = 0;
  for(var i = 0; i < cols; i++) {
    perc_total[0].push(0);
    perc_total[1].push(0);
    for(var j in data) {
      perc_total[1][i] += data[j][i];
      total_total += data[j][i];
    }
  }

  if (total_total > 0) {
    for(var i = 0; i < perc_total[1].length; i++) {
      perc_total[0][i] = (perc_total[1][i] * 100 / total_total) | 0;
    }
  }

  return perc_total;
}

function generate_pdf(options){
  var num_page = 1;
  var num_header = 0;
  var temp_header;
  var doc = new jsPDF();
  num_page++;
  doc.setFont(options.font);
  first_page_edit(doc, options.title, options.date_from, options.date_to, options.report_created, options.author, options.location, options.file, options.comments);
  add_created_with(doc, options.created_with);

  add_new_page(doc, num_page, options.created_with);
  num_page++;
  mid_line(doc);
  if(options.summary == true){
    num_header++;
    create_summary(doc, options.date_from, options.date_to, options.direction, options.total_vehicles, options.average_speed, options._85th_percentile, options._50th_percentile, options._70th_percentile, options.pace_10kmh, options.speed_limit, options.violations_count, num_header)
  }
  if(options.speed_histogram == true){
    num_header++;
    var data = [];
    var max = 0;
    
    for(var j = 0; j < options.speed_histo_data.length; j++){
      var temp = [];
      temp.push(options.speed_histo_data[j].span);
      temp.push(options.speed_histo_data[j].value);
      data.push(temp);
      if (max < options.speed_histo_data[j].value){
        max = options.speed_histo_data[j].value;
      }
    }

    var fixed_max = max;
    for(var i = 1; i < 100; i++) {
      if (max > Math.pow(10, i))
        fixed_max = Math.pow(10, i) * 2;
      if (max > Math.pow(10, i) * 2)
        fixed_max = Math.pow(10, i) * 5;
      if (max > Math.pow(10, i) * 5)
        fixed_max = Math.pow(10, i) * 10;
    }

    speed_histogram(doc, fixed_max, data, options.speed_histogram_color, num_header);
  }
  if ((num_header % 2 == 0) && (num_header != temp_header)){
    temp_header = num_header;
    add_new_page(doc, num_page, options.created_with);
    num_page++;
    mid_line(doc);
  }
  if(options.speed_limit_compliance == true){
    num_header++;
    speed_lim_comp(doc, options.speed_limit_compliance_data, num_header);
  }

  if ((num_header % 2 == 0) && (num_header != temp_header) && (options.compliance_by_hour == true)){
    temp_header = num_header;
    add_new_page(doc, num_page, options.created_with);
    num_page++;
    mid_line(doc);
  }

  if(options.compliance_by_hour == true){
    num_header++;
    var data = [];
    var additional_data = [];
    var max = 0;
    
    for(var j in options.compliance_by_hour_data){
      var temp = [];
      var temp2 = [];

      temp.push(options.compliance_by_hour_data[j].hour);
      temp.push(options.compliance_by_hour_data[j].ok);
      data.push(temp);
      temp2.push(options.compliance_by_hour_data[j]._5);
      temp2.push(options.compliance_by_hour_data[j]._10);
      temp2.push(options.compliance_by_hour_data[j].above);
      additional_data.push(temp2);
    }
    compliance_by_hour(doc, options.comp_by_hour_count_to, data, additional_data, options.comp_by_hour_color, num_header);
  }

  if (num_header % 2 == 0 && num_header != temp_header && (options.compliance_by_hour == true)){
    temp_header = num_header;
    add_new_page(doc, num_page, options.created_with);
    num_page++;
    mid_line(doc);
  }

  if(options.compliance_by_hour == true && options.violations_by_hour == true){
    num_header++;
    var data = [];
    var max = 0;
    
    for(var j in options.compliance_by_hour_data){
      var temp = [];
      temp.push(options.compliance_by_hour_data[j].hour);
      var sum = options.compliance_by_hour_data[j].above + options.compliance_by_hour_data[j]._5 + options.compliance_by_hour_data[j]._10;
      temp.push(sum);
      data.push(temp);
      if (max < sum){
        max = sum;
      }
    }

    var fixed_max = max;
    for(var i = 1; i < 100; i++) {
      if (max > Math.pow(10, i))
        fixed_max = Math.pow(10, i) * 2;
      if (max > Math.pow(10, i) * 2)
        fixed_max = Math.pow(10, i) * 5;
      if (max > Math.pow(10, i) * 5)
        fixed_max = Math.pow(10, i) * 10;
    }


    violations_by_hour(doc, fixed_max, data, options.violations_by_hour_color, num_header)
  }

  if(options.vehicles_by_speed_hour == true){
    num_header++;
    add_new_page(doc, num_page, options.created_with);
    num_page++;
    vehicles_by_speed_hour(doc, num_header, options.hours, options.speed_intervals, options.vehicles_by_speed_hour_data, options.vehicles_by_speed_hour_percentage_total, options.description_vehicles_by_speed_hour);

  }

  if(options.vehicles_by_hour_day == true){
    num_header++;
    num_page = vehicles_by_hour_day(doc, num_header, options.created_with, num_page, options.hours, options.days, options.vehicles_by_hour_day_data, options.vehicles_by_hour_day_percentage_total);
  }

  if(options.violations_by_hour_day == true){
    num_header++;
    num_page = violations_by_hour_day(doc, num_header, options.created_with, num_page, options.hours, options.days, options.violations_by_hour_day_data, options.violations_by_hour_day_percentage_total);
  }

  if(options.vehicles_by_hour_weekday == true){
    num_header++;
    add_new_page(doc, num_page, options.created_with);
    num_page++;
    vehicles_by_hour_weekday(doc, num_header, options.hours, options.weekdays, options.vehicles_by_hour_weekday_data, options.vehicles_by_hour_weekday_percentage_total);
  }

  doc.output('save', 'report.pdf');
};

check_token();

update_files_list();