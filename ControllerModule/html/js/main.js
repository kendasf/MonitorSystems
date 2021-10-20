var map_ctrl = null;
var connection_error_popup = null;
var radar_screen_paper = null;
var targets_overlay = null;
var trace_overlay = null;
var persistance_overlay = null;
var radar_screen_transform = null;
var radar_range = 150;

var intrusions_chart = null;
var fetch_radar_signals = false;

var traces = {};
var persist_state = {
	active: false,
	logs: {}
};
var persist_timeout_ms = 5 * 60 * 1000;

var current_speed_unit = "km/h";

$("#navbar-gps").hide();

start_fetching_targets = function() {
	setTimeout(update_targets_http, 500);
}

start_fetching_calibration_status = function() {
	setInterval(update_calibration_status_http, 500);
}

var last_update_time = 0; 

update_targets_http = function() {
	var delta_t = (+new Date()) - last_update_time;                                                                                                        
  if (delta_t < 150) {
	  setTimeout(update_targets_http, 150 - delta_t);
	  return;
  }
	last_update_time = +new Date();     

	$.ajax({
		url:"/scan_radars?request_rms_values=" + (fetch_radar_signals ? 1 : 0),
		success: on_scan_radars,
		error: function() {
			on_connection_failed();
			update_targets_http();
		}
	});
}

update_calibration_status_http = function() {
	$.ajax({
		url:"/radar_calibration?mode=status",
		success: on_calibration_status,
		error: on_connection_failed
	});
}

on_connection_failed = function() {
	if (!connection_error_popup) {
		$('#connection-failed-popup').modal({keyboard: false, backdrop: 'static'});
		connection_error_popup = true;
	}
}

on_scan_radars = function(result) {
	if (connection_error_popup) {
		connection_error_popup = false;
		$('#connection-failed-popup').modal('hide');
	}

	try {
		update_targets(result.targets, result.ui_units);
		update_signals(result.signals);
	}
	catch (e) {

	}	
	update_targets_http();
}

update_targets = function(targets, units) {
	if (targets_overlay) {
		targets_overlay.remove();
		targets_overlay = null;
	}

	if (trace_overlay) {
		trace_overlay.remove();
		trace_overlay = null;
	}

	if (persistance_overlay) {
		persistance_overlay.remove();
		persistance_overlay = null;
	}

	if (units == "mph") {
		$("#dist-unit").html("ft");
		$("#speed-unit").html("mph");
	}
	else {
		$("#dist-unit").html("m");
		$("#speed-unit").html("km/h");	
	}

	persistance_overlay = radar_screen_paper.path(draw_persistance());
	persistance_overlay.attr({"fill":"white", "fill-opacity":"0.35", "stroke":"none"});
	persistance_overlay.transform(radar_screen_transform);

	if (targets.length > 0) {
		play_alarm();
	}
	else {
	    $("#targets-list-data").html("");
		return;
	}

	targets.sort(function(a, b) { return a.id - b.id });

	var targets_vis_path = "";
	var targets_vis_trace = "";
	var targets_list_str = "";
	var target_ids = {};
	for(var t in targets) {
		var paths = draw_target(targets[t]);
		targets_vis_path += paths[0];
		targets_vis_trace += paths[1];
		targets_list_str += "<tr><td>" + targets[t].id + "</td>";
		var distance = targets[t].distance;
		var speed = targets[t].speed;
		if (units == "mph") {
			distance = distance * 3.28084;
			speed = speed * 0.621371;
		}
		targets_list_str += "<td>" + distance.toFixed(0) + "</td><td>" + targets[t].angle.toFixed(1) + "</td><td>" + speed.toFixed(1) + "</td></tr>";
		target_ids[targets[t].id] = true;

		add_persist_target(targets[t]);
	}

	for(var t in traces) {
		if (!target_ids[t])
			delete traces[t];
	}

	trace_overlay = radar_screen_paper.path(targets_vis_trace);
	trace_overlay.attr({"fill": "none", "stroke-width" : "2", "stroke": document.theme_colors.error, "stroke-opacity": "0.75"});
	trace_overlay.transform(radar_screen_transform);

	targets_overlay = radar_screen_paper.path(targets_vis_path);
	targets_overlay.attr({"fill": document.theme_colors.error, "stroke-width" : "1", "stroke": "#fff", "stroke-opacity": "0.75"});
	targets_overlay.transform(radar_screen_transform);

	$("#targets-list-data").html(targets_list_str);
}

update_signals = function(signals) {
	var max = 1 << 24;
	if (signals) {
		$("#signal-rx1-re").css("width", "" + ((signals.rx1_re * 100 / max)|0) + "%");
		$("#signal-rx1-im").css("width", "" + ((signals.rx1_im * 100 / max)|0) + "%");
		$("#signal-rx2-re").css("width", "" + ((signals.rx2_re * 100 / max)|0) + "%");
		$("#signal-rx2-im").css("width", "" + ((signals.rx2_im * 100 / max)|0) + "%");

		$("#signal-rx1-re-txt").text(signals.rx1_re);
		$("#signal-rx1-im-txt").text(signals.rx1_im);
		$("#signal-rx2-re-txt").text(signals.rx2_re);
		$("#signal-rx2-im-txt").text(signals.rx2_im);
	}
	else {
		$("#signal-rx1-re").css("width", "0%");
		$("#signal-rx1-im").css("width", "0%");
		$("#signal-rx2-re").css("width", "0%");
		$("#signal-rx2-im").css("width", "0%");

		$("#signal-rx1-re-txt").text("");
		$("#signal-rx1-im-txt").text("");
		$("#signal-rx2-re-txt").text("");
		$("#signal-rx2-im-txt").text("");
	}
}

radar_polar_to_cartesian = function(distance, angle) {
	var result = {};
	result.x = distance * Math.sin(Raphael.rad(angle));
	result.y = distance * Math.cos(Raphael.rad(angle));
	return result;
}

radar_to_graph_coords = function(pt) {
	var w = radar_screen_paper.width;
	var h = radar_screen_paper.height;
	var s = h / radar_range;

	var result = {};
	result.x = w / 2 + s * pt.x;
	result.y = h - s * pt.y;	

	return result;
}

draw_target = function(t) {
	var w = radar_screen_paper.width;
	var h = radar_screen_paper.height;
	var s = h / radar_range;

	var graph_pt = radar_to_graph_coords(radar_polar_to_cartesian(t.distance, t.angle));
	var r = 6;

	var target_path = Raphael.format("M{0},{1}a{2},{2},0,1,0,0,0.0001Z", graph_pt.x + r, graph_pt.y , r);
	var trace_path = "";

	for(var tr in traces[t.id]) {
		graph_pt = radar_to_graph_coords(radar_polar_to_cartesian(traces[t.id][tr].distance, traces[t.id][tr].angle));
		if (tr == 0)
			trace_path += Raphael.format("M{0},{1}", graph_pt.x, graph_pt.y);
		else
			trace_path += Raphael.format("L{0},{1}", graph_pt.x, graph_pt.y);
	}

	if (!traces[t.id])
		traces[t.id] = [];
	traces[t.id].push({distance : t.distance, angle: t.angle});

	return [target_path, trace_path];
}

draw_persistance = function() {
	var w = radar_screen_paper.width;
	var h = radar_screen_paper.height;
	var s = h / radar_range;
	var path = "";

	if (!persist_state.active)
		return path;

	var crnt_time_ms = +new Date();
	for(var i in persist_state.logs) {
		for(var j in persist_state.logs[i]) {
			if ((crnt_time_ms - persist_state.logs[i][j]) < persist_timeout_ms) {
				var x = w / 2 + s * i / 2;
				var y = h - s * j / 2;	
				path += Raphael.format("M{0},{1}a{2},{2},0,1,0,0,0.0001Z", x + s * 1.5, y , s * 1.5);
			}
		}
	}

	return path;
}

add_persist_target = function(t) {
	if (!persist_state.active)
		return;

	var x = (2 * t.distance * Math.sin(Raphael.rad(t.angle))) | 0;
	var y = (2 * t.distance * Math.cos(Raphael.rad(t.angle))) | 0;

	if (!persist_state.logs[x])
		persist_state.logs[x] = {};
	
	persist_state.logs[x][y] = +new Date();
}

var intrusion_active = false;
var intrusion_active_timeout = null;

play_alarm = function() {
	switch (document.config.audio_alarm) {
		case 'none':
			return;
		case 'once':
			if (intrusion_active_timeout) {
				clearTimeout(intrusion_active_timeout);
				intrusion_active_timeout = null;
			}
			intrusion_active_timeout = setTimeout(function() {
				intrusion_active = false;
			}, 2000);
			if (intrusion_active)
				return;
			intrusion_active = true;
			break;
		case 'keep':
			if (intrusion_active)
				return;
			setTimeout(function() {
				intrusion_active = false;
			}, 2500);			
			intrusion_active = true;
			break;
	}

	var warning_audio = new Audio('/snd/warning.wav');
	warning_audio.play();
}


setup_radar_screen = function() {
	var holder = $('#radar-screen');
	var parent = $('#radar-screen-parent');
	var heading = $('#radar-screen-heading');

	var width = holder.width();
	var height = parent.height() - heading.outerHeight() - (holder.outerHeight() - holder.height());

	if (radar_screen_paper)
		radar_screen_paper.remove();

	radar_screen_paper = Raphael('radar-screen', width, height);
	radar_screen_paper.padding = holder.outerHeight() - holder.height();
	if (($(window).width() <= 768) || ($(window).height() <= 725)) {
		height = $(window).height() / 2;
		holder.height(height);
	}

	draw_radar_beam(radar_screen_paper, width, height, radar_range, 40);
}

setup_chart_dimensions = function() {
	var holder = $('#chart-intrusions-holder');
	var parent = $('#chart-intrusions-parent');
	var heading = $('#chart-intrusions-heading');

	var width = holder.width();
	var height = parent.height() - heading.outerHeight() - (holder.outerHeight() - holder.height());

	if (($(window).width() <= 768) || ($(window).height() <= 725)) {
		height = $(window).height() / 4;
	}

	holder.height(height);
}

match_panels_size = function () {
	var rsp = $('#radar-screen-parent');
	var tlp = $('#target-list-parent');

	if ($(window).width() > 768)
		tlp.height(rsp.height());

	var cip = $('#chart-intrusions-parent');
	var rsp = $('#radar-status-parent');
	if ($(window).width() > 768)
		rsp.height(cip.height());
}

draw_roi = function(paper, length, tr) {
	if ((!document.config.roi) || (document.config.roi.length < 1))
		return;

	var w = paper.width;
	var h = paper.height;
	var s = h / length;

	for (var i = 0; i < document.config.roi.length; i++) {
		var target_path = "";

		for (var j = 0; j < 5; j++) {
			var lat = document.config.roi[i]["lat" + (j + 1)];
			var lon = document.config.roi[i]["lon" + (j + 1)];

			if ((lat < 0.01) || (lon < 0.01))
				continue;

			var pt = radar_to_graph_coords(gps_to_radar( 
				{ lat: lat, lon: lon }, 
				{ lat: document.config.fixed_latitude, lon: document.config.fixed_longitude }, document.config.fixed_heading));

			if (j == 0)
				target_path += Raphael.format("M{0},{1}", pt.x, pt.y);
			else
				target_path += Raphael.format("L{0},{1}", pt.x, pt.y);
		}

		target_path += "z";

		var p = paper.path(target_path);
		p.attr({ "stroke": document.theme_colors.error, "fill": document.theme_colors.error, "fill-opacity": "0.2", "stroke-width" : "3", "stroke-opacity": "0.8"});
		p.transform(tr);
	}

	
}

draw_radar_beam = function(paper, width, height, length, fov) {
	var grid_length = 25;
	var scale = height / length;
	length -= scale;

	var tr = "";

	var max_w = length * Math.cos(fov * Math.PI / 360);
	if (max_w > width)
		tr = Raphael.format("S{0},{0},{1},{2}", width / max_w, width / max_w, width / 2, height / 2);

	if (width > height) {
		var s = width / height;
		if (max_w * width / height > height)
			s *= height / (max_w * width / height);
		tr = Raphael.format("R90,{0},{1}s{2},{2},{0},{1}", width / 2, height / 2, s);
	}

	tr += Raphael.format("S0.9,0.9,{0},{1}", width / 2, height / 2);

	radar_screen_transform = tr;

	draw_radar_beam_arc(paper, width, height, length * scale, length * scale, fov,
			{ "fill": document.theme_colors.ok, "fill-opacity": "1", "stroke-width" : "1", "stroke": "#fff", "stroke-opacity": "0.75"},
			tr
		);

	for(var i = 0; i < (length / grid_length); i+= 2)  {
		var l = (i + 1) * grid_length;
  	if (l > length) 
			l = length;
		
		draw_radar_beam_arc(paper, width, height, grid_length * scale, l * scale, fov,
			{ "fill": "#fff", "fill-opacity": "0.3", "stroke-width" : "0"},
			tr
		);
		
  }

  draw_roi(paper, length, tr);
}

draw_radar_beam_arc = function(paper, width, height, grid_length, l, fov, attr, tr) {
	var fov_2_rad = fov * Math.PI / 360;
	var p = paper.path(Raphael.format("M{0},{1}A{2},{3},{4},{5},{6},{7},{8}L{9},{10}A{11},{12},{13},{14},{15},{16},{17}z",
		width / 2 - (l - grid_length) * Math.sin(fov_2_rad), height - (l - grid_length) * Math.cos(fov_2_rad),
		l - grid_length, l - grid_length, 0, 0, 1, 
		width / 2 + (l - grid_length) * Math.sin(fov_2_rad), height - (l - grid_length) * Math.cos(fov_2_rad),

		width / 2 + l * Math.sin(fov_2_rad), height - l * Math.cos(fov_2_rad),
		l, l, 0, 0, 0, 
		width / 2 - l * Math.sin(fov_2_rad), height - l * Math.cos(fov_2_rad)
		));

	p.attr(attr);
	p.transform(tr);
}

setup_radar_status = function() {
	setInterval(update_radar_status_http, 1000);
	update_radar_status_http();
}

update_radar_status_http = function() {
	$.ajax({
		url:"/get_status",
		success: on_update_radar_status,
		error: on_connection_failed
	});
}

on_update_radar_status = function(result) {
	try {
		$("#status-version").text(result.status.fw_version + " (" + result.status.fw_date + ")");
		$("#status-dsp-version").text(result.status.dsp_version);
		$("#status-uptime").text(format_uptime(result.status.system_uptime ? result.status.system_uptime : result.status.uptime));
		$("#status-ip-address").text(result.config.ip_address);
		$("#status-monitoring-station").text(result.config.udp_targets_mirror_dest);		

		if (result.status.gps.timestamp == null) {
			$("#navbar-gps").hide();
			$("#status-gps-data").text("GPS receiver not present.");
			$("#status-gps-time").text("-");
		}
		else {
	  	$("#navbar-gps").show();
			if (result.status.gps.hdop == 99) {
				$("#status-gps-data").text("Scanning satellites...");
				$("#status-gps-time").text("-");
			}
			else {
				$("#status-gps-data").text(format_geopos(result.status.gps.latitude, 'N', 'S') + " " + format_geopos(result.status.gps.longitude, 'E', 'W'));	
				$("#status-gps-time").text(format_time(new Date(result.status.gps.timestamp * 1000)));
			}
		}

		if (result.status.active_modules['zigbee'])
			$("#status-zigbee").text("Active.");
		else
			$("#status-zigbee").text("ZigBee module not present.");

		if (result.status.sensors && result.status.sensors.compass && result.status.sensors.compass.valid)
			$("#status-heading").text(result.status.sensors.compass.heading.toFixed(1) + "°");
		else
			$("#status-heading").text("Compass sensor not present.");

		if (result.status.sensors && result.status.sensors.mems && result.status.sensors.mems.valid)
			$("#status-tilt").text("Fwd: " + result.status.sensors.mems.tilt.toFixed(1) + "°" + ", Side:" + result.status.sensors.mems.side_tilt.toFixed(1) + "°");
		else
			$("#status-tilt").text("Tilt sensor not present.");

		if (result.status.sensors)
			$("#status-temperature").text(result.status.sensors.temperature.toFixed(1) + "°C");
		else
			$("#status-temperature").text("-");

		if (result.stats && result.stats.objects)
			$("#stats-object-1").text(result.stats.objects);
		else
			$("#stats-object-1").text("-");

		var units_conv = (result.config.units == 'mph') ? 0.621371 : 1;
		current_speed_unit = result.config.units == 'mph' ? 'mph' : 'km/h';	

		if (result.stats && result.stats.max_speed)
			$("#stats-max-speed").text((result.stats.max_speed * units_conv).toFixed(2) + " " + current_speed_unit);
		else
			$("#stats-max-speed").text("-");

		if (result.stats && result.stats.perc50)
			$("#stats-perc-50").text((result.stats.perc50 * units_conv).toFixed(2) + " " + current_speed_unit);
		else
			$("#stats-perc-50").text("-");

		if (result.stats && result.stats.perc85)
			$("#stats-perc-85").text((result.stats.perc85 * units_conv).toFixed(2) + " " + current_speed_unit);
		else
			$("#stats-perc-85").text("-");


		var chart_data = [];
		var next_idx = [0, 0];
		var mapped = {};
		for(var i = -30; i <= 0; i++) {
			var p = {
				'y': i
			};
			

			if (result.status.graph_tracker.length > -i) {
				var d = result.status.graph_tracker[-i];
				for(var k in d) {
					if (!mapped[k]) {
						var pfx = d[k] < 0 ? 'in' : 'out';
						var ni = d[k] < 0 ? 0 : 1;
						var key = pfx + next_idx[ni];
						var keyX = pfx + 'X' + next_idx[ni];
						next_idx[ni] ++;
						mapped[k] = key;
						if (i > -29)
							p[keyX] = Math.abs(d[k] * units_conv);
					}
					p[mapped[k]] = Math.abs(d[k] * units_conv);
				}
			}

			chart_data.push(p);
		}

		intrusions_chart.setData(chart_data);	
	}
	catch (e) {

	}
}

setup_chart = function() {
	setup_chart_dimensions();

	var data = [];
	for(var i = 0; i <= 30; i++) {
    data.push({
    	'y': -30 + i
    });
  }

  var ykeys = [];
  var lineColors = [];
  for (var i = 0; i < 300; i++) {
		ykeys.push('in' + i);
		ykeys.push('out' + i);
		ykeys.push('inX' + i);
		ykeys.push('outX' + i);
		lineColors.push(document.theme_colors.error);
		lineColors.push(document.theme_colors.primary);
		lineColors.push(document.theme_colors.error);
		lineColors.push(document.theme_colors.primary);
  }

  intrusions_chart = new Morris.Line({
    element: 'chart-intrusions',
    data: data,    
    xkey: 'y',
    ykeys: ykeys,
    labels: ['Speed'],
    yLabelFormat: function(y) { if (!y) return ""; return y.toFixed(0) + " " + current_speed_unit; },
    lineWidth: 1,
    pointSize: [0, 0, 4, 4],
    lineColors: lineColors,
    hideHover: 'always',
    resize: true,
    parseTime: false
  });

  $(window).resize(function() { 
    intrusions_chart.redraw();
  });
}

setup_persist_tracking = function() {
	$("#persist-tracks").click(function() { 
		var checked = !$("#persist-tracks").hasClass("active");
		if (checked) {
			$("#persist-tracks").removeClass("btn-info");
			$("#persist-tracks").addClass("btn-danger");
			persist_state.active = true;
		}
		else {
			$("#persist-tracks").removeClass("btn-danger");
			$("#persist-tracks").addClass("btn-info");
			persist_state.active = false;
			persist_state.logs = {};
		}
	});

	setInterval(clear_vanished_persistance, 5000);
}

clear_vanished_persistance = function() {
	var crnt_time_ms = +new Date();
	var to_del = [];
	for(var i in persist_state.logs) {
		for(var j in persist_state.logs[i]) {
			if ((crnt_time_ms - persist_state.logs[i][j]) > persist_timeout_ms)
				to_del.push({x: i, y: j});
		}
	}

	for(var i = 0; i < to_del.length; i++)
		delete persist_state.logs[to_del[i].x][to_del[i].y];
}

check_calibration = function() {
	if (document.config.sys_calibration) {
		$('#calibration-info').modal({keyboard: false, backdrop: 'static'});
		start_fetching_calibration_status();
	}
	else
		start_fetching_targets();
}

on_calibration_status = function(d) {
	var min = 60;
	var max = 60;

	for(var i = 0; i < d.length; i++) {
		if (i == 0) {
			min = d[i].dsp_temp;
			max = d[i].dsp_temp;
			continue;
		}

		if (d[i].dsp_temp < min)
			min = d[i].dsp_temp;
		if (d[i].dsp_temp > max)
			max = d[i].dsp_temp;
	}

	var perc1 = 0;
	if (min > -20)
		perc1 = (100 * (min + 20) / 80) | 0;

	var perc3 = 0;
	if (max < 60)
		perc3 = (100 * (60 - max) / 80) | 0;

	var perc2 = 100 - perc1 - perc3;

	$("#calibration-progress-1").css("width", "" + perc1 + "%");
	$("#calibration-progress-2").css("width", "" + perc2 + "%");
	$("#calibration-progress-3").css("width", "" + perc3 + "%");
}

setup_signal_monitoring = function() {
	$('a[data-toggle="tab"]').on('shown.bs.tab', function (e) {
		if (e.target == $("#tab-signals-button")[0])
			fetch_radar_signals = true;
		else if (e.relatedTarget == $("#tab-signals-button")[0])
			fetch_radar_signals = false;
	});
}

$(window).resize(function() {
	setup_radar_screen();
	setup_chart_dimensions();
	match_panels_size();
});

setup_persist_tracking();
setup_radar_screen();
setup_radar_status();
setup_chart();
check_calibration();
setup_signal_monitoring();

//setTimeout(match_panels_size, 0);
setTimeout(function() {
	setup_radar_screen();
	setup_chart_dimensions();
}, 10);
setTimeout(match_panels_size, 500);
