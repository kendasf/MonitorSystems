var first_page_edit = function(doc, title, date_from, date_to, report_created, author, location, file, comments){
	doc.setFontSize(50);
	doc.line(20, 80, 190, 80);
	//moram popraviti
	doc.text(title, 105, 95, 'center');
	doc.line(20, 100, 190, 100);

	
	doc.setLineWidth(0.75);
	doc.rect(20, 237, 170, 35);
	

	doc.setFontSize(15);
	//Time interval
	var date_interval = date_from + " - " + date_to;
	doc.setLineWidth(0);
	doc.rect(20, 237, 102, 7);
	doc.text("Time interval: " + date_interval, 21, 242);
	

	//Report created
	doc.setLineWidth(0);
	doc.rect(122, 237, 68, 7);
	doc.text("Report created: " + report_created, 123, 242);
	
	
	//Location
	doc.setLineWidth(0);
	doc.rect(20, 244, 85, 7);
	doc.text("Location: " + location, 21, 249);
	

	//Author
	doc.setLineWidth(0);
	doc.rect(105, 244, 85, 7);
	doc.text("Author: " + author, 106, 249);
	

	//File
	doc.setLineWidth(0);
	doc.rect(20, 251, 170, 7);
	doc.text("File: " + file, 21, 256);
	

	//Comments
	doc.text("Comments: " + comments, 21, 263);
	
}

var add_new_page = function(doc, num_page, created_with){
	doc.addPage();
	top_bot_lines(doc, num_page);
	add_created_with(doc, created_with);
}

var top_bot_lines = function(doc, num_page){

	doc.line (20,25,190,25);
	doc.line (20, 272, 190, 272);
	doc.setFontSize(12);
	doc.setFontStyle("normal");
	doc.text(num_page.toString(), 188, 278);

}

var mid_line = function(doc){
	doc.line (20, 149, 190, 149);
}

var add_created_with = function(doc, created_with){
	doc.setFontSize(12);
	doc.setFontStyle("italic");
	doc.text(created_with, 105, 280, 'center');
}

var create_summary = function(doc, date_from, date_to, direction, total_vehicles, average_speed, perc85, perc50, perc70, pace10, limit, violations_count, num_header){
	doc.setFontSize(25);
	doc.setFontStyle("normal");

	doc.text(num_header + ". Summary", 22, 35);

	doc.setFontSize(13);
	doc.setFontStyle("bold");
	doc.text ("Start date:", 22, 50);
	doc.text ("End date:", 22, 55);
	doc.text ("Total vehicles:", 22, 60);
	doc.text ("Average speed:", 22, 65);
	doc.text ("85th percentile:", 22, 70);
	doc.text ("50th percentile:", 22, 75);
	doc.text ("70th percentile:", 22, 80);
	doc.text ("10 mph pace:", 22, 85);
	doc.text ("Speed limit:", 22, 95);
	doc.text ("Violations count:", 22, 100);

	doc.setFontStyle("normal");
	doc.text (date_from, 70, 50);
	doc.text (date_to, 70, 55);
	doc.text (total_vehicles, 70, 60);
	doc.text (average_speed, 70, 65);
	doc.text (perc85, 70, 70);
	doc.text (perc50, 70, 75);
	doc.text (perc70, 70, 80);
	doc.text (pace10, 70, 85);
	doc.text (limit, 70, 95);
	doc.text (violations_count, 70, 100);
}

var handle_num_header = function(num_header){
	if (num_header % 2 == 0) {
		return 124;
	}
	else{
		return 0;
	}
}

var speed_histogram = function(doc, count_to, data, color, num_header){
	doc.setFontSize(25);
	doc.setFontStyle("normal");

	var y_step = handle_num_header(num_header);

	doc.text(num_header + ". Speed Histogram", 22, 35 + y_step);

	pdf_draw_bar_chart (doc, 35, 46 + y_step, 185, 126 + y_step, data, true, count_to, color, true, null, null);

	doc.setFontSize(12);
	doc.setFontStyle("bold");
	doc.text("Count", 25, 91 + y_step, 90);
	doc.text("Speed", 110, 136 + y_step, "center");

}

var speed_lim_comp = function(doc, data, num_header){
	doc.setFontSize(25);
	doc.setFontStyle("normal");
	var y_step = handle_num_header(num_header);

	doc.text(num_header + ". Speed Limit Compliance", 22, 35 + y_step);
	pdf_draw_pie_chart(doc, 105, 87 + y_step, 30, data);	

	doc.setLineWidth(0.05);
	doc.rect(25, 130 + y_step, 155, 15);
	var x = 27;
	var y = 133;
	doc.setFontSize(9);
	for(var i = 0; i < data.length; i++){
		doc.setFillColor(data[i].color[0], data[i].color[1], data[i].color[2]);
		
		doc.rect(x, y + y_step, 10, 3, 'F');
		doc.text(data[i].name + " (" + data[i].percentage.toString() + "%)", x + 12, y + y_step + 2.5);
		if (i % 2 == 1){
			x = 27;
			y += 6;
		}
		else{
			x += 77;
		}
	}
}

var compliance_by_hour = function(doc, count_to, data, additional_data, color, num_header){
	doc.setFontSize(25);
	doc.setFontStyle("normal");

	var y_step = handle_num_header(num_header);

	var additional_color = [];
	additional_color.push(color.at_or_below_speed_limit_plus5kmh);
	additional_color.push(color.at_or_below_speed_limit_plus10kmh);
	additional_color.push(color.above_speed_limit);

	doc.text(num_header + ". Compliance by Hour", 22, 35 + y_step);

	pdf_draw_bar_chart (doc, 35, 46 + y_step, 185, 126 + y_step, data, false, count_to, color.at_or_below_speed_limit, false, additional_data, additional_color);

	doc.setFontSize(12);
	doc.setFontStyle("bold");
	doc.text("Count", 25, 91 + y_step, 90);
	doc.text("Hour", 110, 127 + y_step, "center");

	doc.setLineWidth(0.05);
	doc.rect(25, 130 + y_step, 155, 15);
	var x = 27;
	var y = 133;
	var i = 0;
	doc.setFontSize(9);
	doc.setFontStyle("normal");
	additional_color.unshift(color.at_or_below_speed_limit);
	var all_colors = additional_color;
	var names = ["At or below speed limit", "At or below speed limit + 5 mph", "At or below speed limit + 10 mph", "Above speed limit"];
	
	for (var j = 0; j < all_colors.length; j++){

		doc.setFillColor(all_colors[j][0], all_colors[j][1], all_colors[j][2]);
		doc.rect(x, y + y_step, 10, 3, 'F');
		doc.text(names[j], x + 12, y + y_step + 2.5);
		if (i % 2 == 1){
			x = 27;
			y += 6;
		}
		else{
			x += 77;
		}
		i++;
	}
}

var violations_by_hour = function(doc, count_to, data, color, num_header){
	doc.setFontSize(25);
	doc.setFontStyle("normal");

	var y_step = handle_num_header(num_header);

	doc.text(num_header + ". Violations by Hour", 22, 35 + y_step);

	pdf_draw_bar_chart (doc, 35, 46 + y_step, 185, 126 + y_step, data, true, count_to, color, false, null, null);

	doc.setFontSize(12);
	doc.setFontStyle("bold");
	doc.text("Count", 25, 91 + y_step, 90);
	doc.text("Hour", 110, 127 + y_step, "center");

	doc.setFontSize(9);
	doc.setFontStyle("normal");
	doc.setLineWidth(0.05);
	doc.rect(25, 131 + y_step, 40, 7);

	doc.setFillColor(color[0], color[1], color[2]);
	doc.rect(27, 133 + y_step, 10, 3, 'F');
	doc.text("Above speed limit", 27 + 12, 133 + y_step + 2.5);
}

var pdf_draw_bar_chart = function(doc, top_left_x, top_left_y, bot_right_x, bot_right_y, data, write_label, count_to, color, rotate, additional_data, additional_color){

	var delta_x = - top_left_x + bot_right_x;
	var delta_y = - top_left_y + bot_right_y;
	var margin_left = 0.05 * delta_x;
	var margin_bot = 0.1 * delta_y;

	var min_y = 0;
	var max_y = count_to;

	//var hor = margin_left + top_left_x;
	var len = delta_y - 2*margin_bot;
	var delta_y_y = Math.abs(max_y - min_y);
	var pix = len / delta_y_y;

	var step = margin_left / 2;
	var blank = delta_x - 2 * margin_left - (data.length + 1) * step;
	var rect_width = blank / data.length;

	doc.setFontSize(10);

	doc.line(top_left_x + margin_left, bot_right_y - margin_bot, top_left_x + margin_left, top_left_y + margin_bot);

	var y_num_parts = 5;
	var x = top_left_x + margin_left + step;
	var y = bot_right_y - margin_bot;
	var y_step = len / y_num_parts;

	var y_side_values = [];

	for (var i = 0; i <= y_num_parts; i++){
		y_side_values.push(0 + i* max_y/y_num_parts);
		doc.line(top_left_x + margin_left, y - i*y_step, top_left_x + margin_left - doc.internal.getFontSize()/3, y - i*y_step);
		doc.text(y_side_values[i].toString(), top_left_x + margin_left - doc.internal.getFontSize()/2, y - i*y_step + doc.internal.getFontSize()/10, "right");

	}

	for(var i = 0; i < data.length; i++) {
		
		doc.setFontSize(10);
		//doc.setDrawColor(0);
		doc.setFillColor(color[0], color[1], color[2]);

		doc.rect(x, bot_right_y - margin_bot, rect_width, -(data[i][1]*pix), 'F'); 
		
		doc.line(x + rect_width/2, bot_right_y - margin_bot + doc.internal.getFontSize()/6, x + rect_width/2, bot_right_y - margin_bot - doc.internal.getFontSize()/6);
		if (rotate == true){
			doc.text(data[i][0].toString(), x + rect_width/2 + doc.internal.getFontSize()/9, bot_right_y - margin_bot + doc.internal.getFontSize()*1.2, 90);
		}
		else{
			doc.text(data[i][0].toString(), x + rect_width/2, bot_right_y - margin_bot + doc.internal.getFontSize()/2, "center");
		}
		var bar_height = -(data[i][1]*pix);

		if(additional_data != null){
			if(additional_data[i][0] != 0){
				doc.setFillColor(additional_color[0][0],additional_color[0][1], additional_color[0][2]);
				doc.rect(x, bot_right_y - margin_bot + bar_height, rect_width, -(additional_data[i][0]*pix), 'F');
				bar_height = bar_height -(additional_data[i][0]*pix);
			}

			if(additional_data[i][1] != 0){
				doc.setFillColor(additional_color[1][0],additional_color[1][1], additional_color[1][2]);
				doc.rect(x, bot_right_y - margin_bot + bar_height, rect_width, -(additional_data[i][1]*pix), 'F');
				bar_height = bar_height - (additional_data[i][1]*pix);
			}

			if(additional_data[i][2] != 0){
				doc.setFillColor(additional_color[2][0],additional_color[2][1], additional_color[2][2]);

				doc.rect(x, bot_right_y - margin_bot + bar_height, rect_width, -(additional_data[i][2]*pix), 'F');
				bar_height = bar_height -(additional_data[i][2]*pix);
			}
		}

		if (write_label == true && data[i][1] != 0){
			doc.setFontSize(8);
			doc.text(data[i][1].toString(), x + rect_width/2,  bot_right_y - margin_bot - (data[i][1]*pix) - doc.internal.getFontSize()/3, "center");
		}

		x += rect_width + step;
	}

	doc.line(top_left_x + margin_left , bot_right_y - margin_bot, top_left_x + delta_x - margin_left, bot_right_y - margin_bot);
}

var pdf_draw_pie_chart = function(doc, center_x, center_y, radius, data){
	var x0 = center_x;
	var y0 = center_y;
	var x1 = center_x;
	var y1 = center_y - radius;
	var primx = center_x;
	var primy = center_y - radius;
	var start = 0, goal = 0;
	var goals = [];

	for(var j = 0; j < data.length; j++){
		goal = 360 * (data[j].percentage/100);
		goals.push(goal*(Math.PI/180));
	}
		
	goal = 0;
	for(var j = 0; j < data.length; j++){
		var punkts = [];
		punkts.push([primx- center_x, primy -center_y]);
		goal += 360 * (data[j].percentage/100);
		

		for(start; start >= 0 && start < goal  && start < 90; start+=0.1){
			var x2 = Math.sin((start)*(Math.PI/180)) * radius + center_x;
			var y2 = center_y - Math.cos((start)*(Math.PI/180)) * radius;

			punkts.push([x2-x1, y2-y1]);

			x1 = x2;
			y1 = y2;	
		}
		
		for(start; start >= 90 && start < goal && start < 180 ; start+=0.1){
			var y2 = Math.sin((start-90)*(Math.PI/180)) * radius + center_y;
			var x2 = center_x + Math.cos((start-90)*(Math.PI/180)) * radius;

			punkts.push([x2-x1, y2-y1]);

			x1 = x2;
			y1 = y2;	
		}
		
		for(start; start >= 180 && start < goal && start < 270; start+=0.1){
			var x2 = -Math.sin((start - 180)*(Math.PI/180)) * radius + center_x;
			var y2 = center_y + Math.cos((start - 180)*(Math.PI/180)) * radius;

			punkts.push([x2-x1, y2-y1]);
	
			x1 = x2;
			y1 = y2;
		}

		for(start; start >= 270 && start < goal && start <= 360; start+=0.1){
			var y2 = -Math.sin((start-270)*(Math.PI/180)) * radius + center_y;
			var x2 = center_x - Math.cos((start-270)*(Math.PI/180)) * radius;

			punkts.push([x2-x1, y2-y1]);

			x1 = x2;
			y1 = y2;	
		}

		var color = data[j].color;
		doc.setFillColor(color[0], color[1], color[2]);
		doc.lines(punkts, center_x, center_y, [1.0, 1.0], 'F', 0);
		primx = x2;
		primy = y2;
	}
}

var vehicles_by_speed_hour = function(doc, num_header, vertical_values, horizontal_values, data, perc_total, description){

	doc.setFontSize(25);
	doc.setFontStyle("normal");
	doc.text(num_header + ". Vehicles by Speed/Hour", 22, 35);

	var x_start = 30;
	var y_start = 50;
	var y_delta = 180/ horizontal_values.length;
	var x_width = 10;
	var xx = x_width / 6;



	doc.setLineWidth(0.4);
	make_horizontal_label(doc, horizontal_values, "Speed (mph)", "Hour", x_start, y_start, y_delta, x_width);
	make_vertical_label(doc, vertical_values, x_start + x_width, y_start + (horizontal_values.length + 2) * y_delta, 2 * y_delta, x_width / 2, xx);
	make_grid(doc, horizontal_values.length, vertical_values.length, x_start, y_start, y_delta, x_width / 2, data, perc_total, xx);

	doc.setFontSize(10);
	doc.setFontStyle("bold");
	doc.text(description, x_start-5, y_start + y_delta*(horizontal_values.length +2), 90);
}

var make_horizontal_label = function(doc, horizontal_values, hor_name, ver_name, x_start, y_start, y_delta, x_width){

	doc.setFontSize(9);
	doc.setFontStyle("bold");
	for(var i = horizontal_values.length - 1; i >= 0; i--){

		doc.rect(x_start, y_start, x_width, y_delta);
		y_start += y_delta;

		doc.text(horizontal_values[i], x_start + x_width/2 + doc.internal.getFontSize()/9, y_start - 2, 90)
		
	}

	if(y_delta < 15){
		y_delta *= 2;
	}
	doc.rect(x_start, y_start, x_width, y_delta);
	doc.line(x_start + x_width/2, y_start + y_delta /2, x_start + x_width/2, y_start + y_delta);
	doc.line(x_start + x_width/2, y_start + y_delta / 2, x_start + x_width, y_start);

	
	doc.setFontStyle("normal");

	doc.setFontSize(11);
	doc.text(hor_name, x_start + x_width/4 +1, y_start + y_delta - 2, 90);
	doc.text(ver_name, x_start + 3*x_width/4 +1, y_start + y_delta - 2, 90);
}

var make_vertical_label = function(doc, vertical_values, x_start, y_start, y_delta, x_width, xx){
	doc.setFontSize(10);

	if(y_delta < 15){
		y_delta *= 2;
	}

	for( var i = 0; i < vertical_values.length; i++){

		doc.rect(x_start, y_start, x_width, -y_delta);

		doc.text(vertical_values[i], x_start + x_width/2 + doc.internal.getFontSize()/9, y_start - 2, 90);

		x_start += x_width;
	}

	
	doc.rect(x_start, y_start, xx, -y_delta);

	doc.rect(x_start + xx, y_start, x_width, -y_delta);
	doc.text("Percentage", x_start + x_width/2 + doc.internal.getFontSize()/9 + xx, y_start - 2, 90);

	doc.rect(x_start + x_width + xx, y_start, x_width, -y_delta);
	doc.text("TOTAL", x_start + 3*x_width/2 + doc.internal.getFontSize()/9 + xx, y_start - 2, 90);

}

var make_grid = function(doc, hor_val_length, ver_val_length, x_start, y_start, y_delta, x_width, data, perc_total, xx){
	var first_x = x_start + 2 * x_width;
	var first_y = y_start + y_delta * (hor_val_length);
	var saver = first_y;
	doc.setLineWidth(0);

	// okrenuta je slika pa treba paziti na strukturu petlje
	for (var i = 0; i < ver_val_length; i++){
		first_y = saver;

		for (var j = 0; j < hor_val_length; j++){
			doc.rect(first_x, first_y, x_width, - y_delta);
			if (data[i] && data[i][j] && data[i][j] != 0) {
				doc.text(data[i][j].toString(), first_x + x_width/2 + doc.internal.getFontSize()/9, first_y - 2, 90);
			}
			first_y -= y_delta;
		}
		first_x += x_width;
	}

	first_y = saver;
	for (var j = 0; j < hor_val_length; j++){
		doc.rect(first_x, first_y, xx, -y_delta);
		first_y -= y_delta;
	}

	first_x += xx;
	for (var i = 0; i < perc_total.length; i++){
		first_y = saver;
		for (var j = 0; j < hor_val_length; j++){
			doc.rect(first_x, first_y, x_width, - y_delta);
			if (perc_total[i][j] != 0){
				doc.text(perc_total[i][j].toString(), first_x + x_width/2 + doc.internal.getFontSize()/9, first_y - 2, 90);
			}
			first_y -= y_delta;
		}
		first_x += x_width;
		doc.setFontStyle("bold");
	}
}

var vehicles_by_hour_day = function(doc, num_header, created_with, num_page, vertical_values, horizontal_values, data, perc_total){
	var entries_per_page = 7;
	for (var page = 0; page < (((horizontal_values.length - 1) / entries_per_page) | 0) + 1; page++) {
		add_new_page(doc, num_page++, created_with);

		if (page == 0) {
			doc.setFontSize(25);
			doc.setFontStyle("normal");
			doc.text(num_header + ". Vehicles by Hour/Day", 22, 35);
		}

		var page_horizontal_values = [];
		var page_data = [];
		var page_perc_total = [[],[]];

		for (var j = page * entries_per_page; (j < horizontal_values.length) && (j < page * entries_per_page + entries_per_page); j++) {
			page_horizontal_values.push(horizontal_values[j]);

			page_perc_total[0].push(perc_total[0][j]);
			page_perc_total[1].push(perc_total[1][j]);

			for (var k = 0; k < 24; k++) {
				if (!page_data[k])
					page_data[k] = [];
				page_data[k][j - page * entries_per_page] = data[k][j];
			}
		}

		var x_start = 30;
		var y_start = 50;
		var y_delta = 180 / page_horizontal_values.length;
		if (y_delta > 25){
			y_delta = 25;
		}
		var x_width = 10;
		var xx = x_width / 6;

		doc.setLineWidth(0.4);
		make_horizontal_label(doc, page_horizontal_values, "Day", "Hour", x_start, y_start, y_delta, x_width);
		make_vertical_label(doc, vertical_values, x_start + x_width, y_start + (page_horizontal_values.length + 1) * y_delta, y_delta, x_width / 2, xx);
		make_grid(doc, page_horizontal_values.length, vertical_values.length, x_start, y_start, y_delta, x_width / 2, page_data, page_perc_total, xx);
	}	

	return num_page;
}

var violations_by_hour_day = function(doc, num_header, created_with, num_page, vertical_values, horizontal_values, data, perc_total){
	var entries_per_page = 7;
	for (var page = 0; page < (((horizontal_values.length - 1) / entries_per_page) | 0) + 1; page++) {
		add_new_page(doc, num_page++, created_with);

		if (page == 0) {
			doc.setFontSize(25);
			doc.setFontStyle("normal");
			doc.text(num_header + ". Violations by Hour/Day", 22, 35);
		}

		var page_horizontal_values = [];
		var page_data = [];
		var page_perc_total = [[],[]];

		for (var j = page * entries_per_page; (j < horizontal_values.length) && (j < page * entries_per_page + entries_per_page); j++) {
			page_horizontal_values.push(horizontal_values[j]);

			page_perc_total[0].push(perc_total[0][j]);
			page_perc_total[1].push(perc_total[1][j]);

			for (var k = 0; k < 24; k++) {
				if (!page_data[k])
					page_data[k] = [];
				if (data[k] && data[k][j])
					page_data[k][j - page * entries_per_page] = data[k][j];
				
			}
		}

		var x_start = 30;
		var y_start = 50;
		var y_delta = 180/ page_horizontal_values.length;
		if (y_delta > 25){
			y_delta = 25;
		}
		var x_width = 10;
		var xx = x_width / 6;

		doc.setLineWidth(0.4);
		make_horizontal_label(doc, page_horizontal_values, "Day", "Hour", x_start, y_start, y_delta, x_width);
		make_vertical_label(doc, vertical_values, x_start + x_width, y_start + (page_horizontal_values.length + 1) * y_delta, y_delta, x_width / 2, xx);
		make_grid(doc, page_horizontal_values.length, vertical_values.length, x_start, y_start, y_delta, x_width / 2, page_data, page_perc_total, xx);
	}

	return num_page;
}

var vehicles_by_hour_weekday= function(doc, num_header, vertical_values, horizontal_values, data, perc_total){
	doc.setFontSize(25);
	doc.setFontStyle("normal");
	doc.text(num_header + ". Violations by Hour/Weekday", 22, 35);

	var x_start = 30;
	var y_start = 50;
	var y_delta = 180/ horizontal_values.length;
	if (y_delta > 25){
		y_delta = 25;
	}
	var x_width = 10;
	var xx = x_width / 6;

	doc.setLineWidth(0.4);
	make_horizontal_label(doc, horizontal_values, "Weekday", "Hour", x_start, y_start, y_delta, x_width);
	make_vertical_label(doc, vertical_values, x_start + x_width, y_start + (horizontal_values.length + 1) * y_delta, y_delta, x_width / 2, xx);
	make_grid(doc, horizontal_values.length, vertical_values.length, x_start, y_start, y_delta, x_width / 2, data, perc_total, xx);
}