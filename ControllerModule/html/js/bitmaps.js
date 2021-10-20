var is_drawing = false;
var draw_color;
var image_width = 0;
var image_height = 0;
var image_pixels = null;
var paper = null;
var current_image = -1;

var default_params = {
  'vertical-align': 'top',
  'line1-text': '',
  'line1-font': 'Narrow 10',
  'line1-align': 'left',
  'line2-text': '',
  'line2-font': 'Narrow 10',
  'line2-align': 'left',
  'line3-text': '',
  'line3-font': 'Narrow 10',
  'line3-align': 'left',
  'line4-text': '',
  'line4-font': 'Narrow 10',
  'line4-align': 'left'
};

var text_params = null;

function create_bitmap_grid() {
  var html = "";

  for (var i = 0; i < 42; i++) {
    html += '<div class="row">';
      for (var j = 0; j < 12; j++) {
        if (i * 12 + j + 1 > 500)
          continue;
        html += '<div class="col-lg-1 col-md-2 col-sm-3 col-xs-4">#' + (i * 12 + j + 1) + '<br/><a href="#" onclick="edit_image(' + (i * 12 + j) + ')"><img id="img-preview-' + (i * 12 + j) + '" src="/get_bitmap?id=' + (i * 12 + j) + '" width="100%"></a></div>';
      }
    html += '</div>';
  }

  $("#bitmap-grid").html(html);
}

function update_bitmap_grid() {
  var token = get_token();
  $.ajax("/read_bitmaps?token=" + token + "&ts=" + (+new Date()) , {
    dataType: "json" 
  }).done(function(response) {
    redraw_bitmap_grid(response);
  });
}

function edit_image(id) {
  var token = get_token();
  $.ajax("/get_bitmap_data?token=" + token + "&id=" + id + "&ts=" + (+new Date()), {
    dataType: "json" 
  }).done(function(response) {
    start_edit(response, id);
  });
}

function start_edit(response, id) {
  if (!response.image_data)
    return;

  image_height = response.image_data.length;
  image_width  = response.image_data[0].length;
  current_image = id;

  try{
    if (paper)
      paper.remove();
  }
  catch (error) {
      // this catches the error and allows you to proceed along nicely
  }

  paper = new Raphael(document.getElementById('image-editor'), image_width, image_height);
  paper.setViewBox(0, 0, image_width * 10, image_height * 10, true);
  paper.setSize('100%', '100%');

  var bkg = paper.rect(0, 0, image_width * 10, image_height * 10);
  bkg.attr( { fill: '#000000'});

  image_pixels = [];
  for (var i = 0; i < image_height; i ++) {
    image_pixels.push([]);
    for (var j = 0; j < image_width; j++) {
      var dot = paper.rect(j * 10, i * 10, 10, 10);
      if (response.image_data[i][j] == 0)
        dot.attr( { fill: '#000000', "fill-opacity": "0", stroke: '#777777', "stroke-opacity": "1", "stroke-width": "1"});
      else
        dot.attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
      dot.bitmap_value = response.image_data[i][j];
      dot.text_value = 0;
      image_pixels[image_pixels.length - 1].push(dot);
    }
  }

  text_params = JSON.parse(JSON.stringify(default_params));

  $('#img-edit-valign-bottom').prop('checked', false);
  $('#img-edit-valign-center').prop('checked', false);
  $('#img-edit-valign-top').prop('checked', true);

  $('#img-edit-valign-top-label').addClass('active');
  $('#img-edit-valign-center-label').removeClass('active');
  $('#img-edit-valign-bottom-label').removeClass('active');

  for (var i = 1; i <= 4; i++) {
    $('#line' + i + '-align-rught').prop('checked', false);
    $('#line' + i + '-align-center').prop('checked', false);
    $('#line' + i + '-align-left').prop('checked', true);

    $('#line' + i + '-align-left-label').addClass('active');
    $('#line' + i + '-align-center-label').removeClass('active');
    $('#line' + i + '-align-right-label').removeClass('active');

    $("#img-edit-line" + i + "-font").text("Narrow 10\"");
  }

  $('#line1-text').val('');
  $('#line2-text').val('');
  $('#line3-text').val('');
  $('#line4-text').val('');

  $("#image-edit-dialog").modal('show');
  $("#image-edit-dialog").css('overflow-y', 'auto');
}

function on_editor_mouseup(e) {
  is_drawing = false;
}

function on_editor_mousedown(e) {
  if ((image_width == 0) || (image_height == 0))
    return;

  var parentOffset = $("#image-editor").offset();

  var offset_x = e.pageX - parentOffset.left;
  var offset_y = e.pageY - parentOffset.top;
  var pixel_x = (image_width * offset_x / $("#image-editor").width()) | 0;

  if ($("#image-editor").width() / $("#image-editor").height() - image_width / image_height > 1) {
    var pixel_size = $("#image-editor").height() / image_height;
    var x_start = ($("#image-editor").width() - image_width * pixel_size) / 2;

    offset_x = e.pageX - parentOffset.left - x_start;
    pixel_x = (offset_x / pixel_size) | 0;
  }

  var pixel_y = (image_height * offset_y / $("#image-editor").height()) | 0;

  if (image_pixels[pixel_y][pixel_x].attrs.fill == "#000000")
    draw_color = true;
  else
    draw_color = false;

  is_drawing = true;

  on_editor_mousemove(e);
}

function on_editor_mousemove(e) {
  if (e.buttons == 0)
    is_drawing = false;
  
  if (!is_drawing)
    return;

  if ((image_width == 0) || (image_height == 0))
    return;

  var parentOffset = $("#image-editor").offset();

  var offset_x = e.pageX - parentOffset.left;
  var offset_y = e.pageY - parentOffset.top;

  var pixel_x = (image_width * offset_x / $("#image-editor").width()) | 0;

  if ($("#image-editor").width() / $("#image-editor").height() - image_width / image_height > 1) {
    var pixel_size = $("#image-editor").height() / image_height;
    var x_start = ($("#image-editor").width() - image_width * pixel_size) / 2;

    offset_x = e.pageX - parentOffset.left - x_start;
    pixel_x = (offset_x / pixel_size) | 0;
  }

  
  var pixel_y = (image_height * offset_y / $("#image-editor").height()) | 0;

  if (draw_color) {
    image_pixels[pixel_y][pixel_x].attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
    image_pixels[pixel_y][pixel_x].bitmap_value = 1;
  }
  else {
    image_pixels[pixel_y][pixel_x].attr( { fill: '#000000', "fill-opacity": "0", stroke: '#777777', "stroke-opacity": "1", "stroke-width": "1"});
    image_pixels[pixel_y][pixel_x].bitmap_value = 0;
    image_pixels[pixel_y][pixel_x].text_value = 0;
  }
}

function image_editor_clear_image() {
  for (var i = 0; i < image_pixels.length; i++) {
    var row = image_pixels[i];
    for (var j = 0; j < row.length; j++) {
      row[j].attr( { fill: '#000000', "fill-opacity": "0", stroke: '#777777', "stroke-opacity": "1", "stroke-width": "1"});
      image_pixels[i][j].bitmap_value = 0;
      image_pixels[i][j].text_value = 0;
    }
  }
  update_text({});
}

function update_text(params) {
  if (params['line1-font'])
    $("#img-edit-line1-font").text(params['line1-font'] + "\"");
  if (params['line2-font'])
    $("#img-edit-line2-font").text(params['line2-font'] + "\"");
  if (params['line3-font'])
    $("#img-edit-line3-font").text(params['line3-font'] + "\"");
  if (params['line4-font'])
    $("#img-edit-line4-font").text(params['line4-font'] + "\"");

  for(var k in params) {
    text_params[k] = params[k];
  }

  var temp_canvas = document.createElement('canvas');
  temp_canvas.width = image_width * 8;
  temp_canvas.height = image_height * 8;

  var temp_context = temp_canvas.getContext('2d');
  temp_context.fillRect(0, 0, image_width * 8, image_height * 8);

  var ypos = 0;
  if (text_params['line1-text'].length > 0) 
    ypos += get_line_height(text_params['line1-font']);
  if (text_params['line2-text'].length > 0)
    ypos += get_line_height(text_params['line2-font']);
  if (text_params['line3-text'].length > 0)
    ypos += get_line_height(text_params['line3-font']);
  if (text_params['line4-text'].length > 0)
    ypos += get_line_height(text_params['line4-font']);

  temp_context.fillStyle = "white";

  var yoffset = 0;
  if (text_params['vertical-align'] == 'center')
    yoffset = (((((image_height - ypos ) | 0) * 4) / 8) | 0 ) * 8;
  if (text_params['vertical-align'] == 'bottom')
    yoffset = ((image_height - ypos ) | 0) * 8; 

  ypos = yoffset;
  if (text_params['line1-text'].length > 0) 
    ypos = render_line(text_params['line1-align'], text_params['line1-font'], text_params['line1-text'], temp_context, ypos);
  if (text_params['line2-text'].length > 0)
    ypos = render_line(text_params['line2-align'], text_params['line2-font'], text_params['line2-text'], temp_context, ypos);
  if (text_params['line3-text'].length > 0)
    ypos = render_line(text_params['line3-align'], text_params['line3-font'], text_params['line3-text'], temp_context, ypos);
  if (text_params['line4-text'].length > 0)
    ypos = render_line(text_params['line4-align'], text_params['line4-font'], text_params['line4-text'], temp_context, ypos);

  var image = temp_context.getImageData(0, 0, image_width * 8, image_height * 8);
  for (var i = 0; i < image_height; i++) {
    for (var j = 0; j < image_width; j++) {
      var pixel_offset = ((i * 8 + 4) * image_width * 8 + j * 8 + 4) * 4;
      if (image.data[pixel_offset] != 0) {
        image_pixels[i][j].attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
        image_pixels[i][j].text_value = 1;
      }
      else {
        image_pixels[i][j].text_value = 0;  
        if (image_pixels[i][j].bitmap_value == 0)
          image_pixels[i][j].attr( { fill: '#000000', "fill-opacity": "0", stroke: '#777777', "stroke-opacity": "1", "stroke-width": "1"});
      }
    }
  }
}

function get_line_height(font) {
  if (font == 'Narrow 7.5') {
    return 8;
  }
  if (font == 'NEMA 7.5') {
    return 8;
  }
  if (font == 'Large 9.5') {
    return 10;
  }
  if (font == 'Narrow 10') {
    return 11;
  }
  if (font == 'Wide 10') {
    return 11;
  }
  if (font == 'Bold 13') {
    return 14;
  }
  return 0;
}

var www = 112;

function render_line(align, font, text, ctx, ypos) {
  var yoffset = 0;
  var xdelta = 0;

  if (font == 'Narrow 7.5') {
    ctx.font = '114px "Narrow7px-Regular"';
    yoffset = 8 * 8;
    xdelta = 1;
  }
  if (font == 'NEMA 7.5') {
    ctx.font = '114px "NEMA5x7-Regular"';
    yoffset = 8 * 8;
    xdelta = 1;
  }
  if (font == 'Large 9.5') {
    ctx.font = '147px "7x9-Regular"';
    yoffset = 10 * 8;
    xdelta = 1;
  }
  if (font == 'Narrow 10') {
    ctx.font = '163px "Geolux10pxNarrow-Regular"';
    yoffset = 11 * 8;
    xdelta = 1;
  }
  if (font == 'Wide 10') {
    ctx.font = '163px "Geolux10pxMonospaced-Regular"';
    yoffset = 11 * 8;
    xdelta = 1;
  }
  if (font == 'Bold 13') {
    ctx.font = '208px "Bold10px-Regular"';
    yoffset = 14 * 8;
    xdelta = 2;
    text = text.toUpperCase();
  }

  var xpos = 0;
  ypos += yoffset;

  var text_width = 0;
  for(var i = 0; i < text.length; i++) {
    text_width += ctx.measureText(text[i]).width + xdelta * 8;
  }
  text_width -= xdelta * 8;

  if (align == 'right') {
    xpos = ((image_width - text_width / 8) * 8 ) | 0;
  }
  if (align == 'center') {
    xpos = ((((image_width * 8 - text_width) / 2) / 8) | 0) * 8;
  }

  for(var i = 0; i < text.length; i++) {
    ctx.fillText(text[i], xpos, ypos);
    xpos += ctx.measureText(text[i]).width + xdelta * 8;
  }

  return ypos;
}

function save_image() {
  var image_data = "";
  var raw_data = [];

  for (var i = 0; i < 48; i++) {
    for (var j = 0; j < 60; j++) {
      var raw_idx = i * 60 + j;

      if (raw_data.length <= (raw_idx / 8))
        raw_data.push(0);

      if ((j >= image_width) || (i >= image_height))
        continue;

      if (image_pixels[i][j].bitmap_value || image_pixels[i][j].text_value) {
        raw_data[(raw_idx / 8) | 0] |= (1 << (raw_idx % 8));
      }
    }
  }

  for (var i = 0; i < raw_data.length; i++) {
    var digit = raw_data[i].toString(16);
    if (digit.length == 1)
      digit = "0" + digit;
    image_data += digit;
  }

  var token = get_token();

  $.ajax("/save_bitmap?token=" + token + "&id=" + current_image + "&data=" + image_data + "&ts=" + (+new Date()), {
    dataType: "json" 
  }).done(function(response) {
    if (response && response.status == 'ok') {
      $("#image-edit-dialog").modal('hide');
      $("#img-preview-" + current_image).attr("src", "/get_bitmap?token=" + token + "&id=" + current_image + "&ts=" + (+ new Date()));
      return;
    }
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

function load_from_library() {
  var token = get_token();
  $.ajax("/enum_library_contents?token=" + token + "&ts=" + (+ new Date()), {
    dataType: "json" 
  }).done(function(response) {
    if (response) {
      on_enum_library_contents(response);
    }
  });
}

function save_to_local() {
  var temp_canvas = document.createElement('canvas');
  temp_canvas.width = image_width;
  temp_canvas.height = image_height;
  var temp_context = temp_canvas.getContext('2d');

  temp_context.fillStyle = "black";
  temp_context.fillRect(0, 0, image_width, image_height);

  temp_context.fillStyle = "white";

  for (var i = 0; i < 48; i++) {
    for (var j = 0; j < 60; j++) {
      if ((j >= image_width) || (i >= image_height))
        continue;

      if (image_pixels[i][j].bitmap_value || image_pixels[i][j].text_value) {
        temp_context.fillRect(j, i, 1, 1);
      }
    }
  }

  temp_canvas.toBlob(function(blob) {
      saveAs(blob, "vms-image-" + (current_image + 1) + ".png");
  });
}

function fix_name(name) {
  var fixed_name = "";

  for (var i = 0; i < name.length; i++) {
    if (name.charAt(i) == '+') {
      if (i + 2 < name.length) {
        var code = parseInt(name.charAt(i + 1) + name.charAt(i + 2), 16);
        fixed_name += String.fromCharCode(code);
        i += 2;
      }
    }
    else {
      fixed_name += name.charAt(i);
    }
  }

  return fixed_name;
}

function on_enum_library_contents(library_names) {
  var html = "";

  for (var i = 0; i < library_names.images.length; i++) {
    if ((i % 4) == 0)
      html += '<div class="row">';

    html += '<div class="col-lg-3 col-md-3 col-sm-3 col-xs-6">' + fix_name(library_names.images[i]) + '<br/><a href="#" onclick="select_library_image(' + i + ')"><img id="img-preview-' + i + '" src="/get_library_bitmap?id=' + i + '" width="100%"></a></div>';

    if ((i % 4) == 3)
      html += '</div>';
  }

  if ((library_names.length - 1) % 4 != 3)
    html += '</div>';

  $("#library-bitmap-grid").html(html);

  $("#select-library-image-dialog").modal('show');
}

function select_library_image(id) {
  $("#select-library-image-dialog").modal('hide');
  $("#image-edit-dialog").css('overflow-y', 'auto');

  var token = get_token();
  $.ajax("/get_library_bitmap_data?token=" + token + "&ts=" + (+ new Date()) + "&id=" + id, {
    dataType: "json" 
  }).done(function(response) {
    if (response) {
      on_set_library_image(response);
    }
  });
}

function on_set_library_image(response) {
  if (response.image_data) {
    for (var i = 0; i < response.image_data.length; i++) {
      for (var j = 0; j < response.image_data[i].length; j++) {
        if (response.image_data[i][j] != 0) {
          image_pixels[i][j].attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
          image_pixels[i][j].bitmap_value = 1;
        }
        else {
          image_pixels[i][j].attr( { fill: '#000000', "fill-opacity": "0", stroke: '#777777', "stroke-opacity": "1", "stroke-width": "1"});
          image_pixels[i][j].bitmap_value = 0;
          image_pixels[i][j].text_value = 0;
        }
      }
    }
  }
}

function load_from_file() {
  $("#select-image-div").html('<input type="file" id="image_files" name="image_files[]" accept="image/*" /><br><canvas id="loaded-image-canvas"></canvas>');
  document.getElementById('image_files').addEventListener('change', handle_file_select, {capture: false });
  $("#select-image-file-dialog").modal('show');
}

function handle_file_select(evt) {
  var files = evt.target.files;

  if (files.length == 0)
    return;

  if (!files[0].type.match('image.*'))
    return;

  var reader = new FileReader();

  reader.onload = function(e) {
    var mem_image = new Image();
    mem_image.src = e.target.result;

    mem_image.onload = function () {
      var canvas = document.getElementById('loaded-image-canvas');
      var context = document.getElementById('loaded-image-canvas').getContext('2d');
      context.clearRect(0, 0, canvas.width, canvas.height);

      context.drawImage(mem_image, 0, 0);
    }
  }

  reader.readAsDataURL(evt.target.files[0]);
}

function use_loaded_image() {
  var canvas = document.getElementById('loaded-image-canvas');
  var context = document.getElementById('loaded-image-canvas').getContext('2d');

  for (var i = 0; i < image_pixels.length; i++) {
    for (var j = 0; j < image_pixels[i].length; j++) {
      var pixel_data = context.getImageData(j, i, 1, 1).data;

      if ((pixel_data[0] != 0) || (pixel_data[1] != 0) || (pixel_data[2] != 0)) {
        image_pixels[i][j].attr( { fill: '#ffc200', "fill-opacity": "1", stroke: 'none', "stroke-opacity": '0'});
        image_pixels[i][j].bitmap_value = 1;
      }
      else {
        image_pixels[i][j].attr( { fill: '#000000', "fill-opacity": "0", stroke: '#777777', "stroke-opacity": "1", "stroke-width": "1"});
        image_pixels[i][j].bitmap_value = 0;
        image_pixels[i][j].text_value = 0;
      }
    }
  }

  $("#select-image-file-dialog").modal('hide');
  $("#image-edit-dialog").css('overflow-y', 'auto');
}


if (window.File && window.FileReader && window.FileList && window.Blob) {
  $("#load-file").html('<button type="button" class="btn btn-default" onclick="load_from_file()">Load from local file...</button>&nbsp;&nbsp;');
  $("#save-file").html('<button type="button" class="btn btn-default" onclick="save_to_local()">Save to local file</button>');
}

check_token(1);

create_bitmap_grid();

$( "#image-editor" ).mousedown(function(e) {
  on_editor_mousedown(e);
});

$( "#image-editor" ).mouseup(function(e) {
  on_editor_mouseup(e);
});

$( "#image-editor" ).mousemove(function(e) {
  on_editor_mousemove(e);
});

setInterval(update_status, 2000);
