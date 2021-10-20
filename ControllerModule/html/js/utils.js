var current_user_level = -1;

format_two_digit = function(t) {
  t = t.toString();
  if (t.length == 0)
    return "00";
  if (t.length == 1)
    return "0" + t;
  return t;
}

format_time = function(d, skip_date) {
  var res = "";
  if (!skip_date)
    res += d.getUTCFullYear() + "-" + (format_two_digit(d.getUTCMonth() + 1)) + "-" + format_two_digit(d.getUTCDate()) + " ";
  res += d.getUTCHours() + ":" + format_two_digit(d.getUTCMinutes()) + ":" + format_two_digit(d.getUTCSeconds()) + " UTC";
  return res;
}

format_size = function(size) {
  if (size < 1024)
    return size + " B";
  if (size < 1024 * 1024)
    return (size / 1024).toFixed(1) + " kB";
  if (size < 1024 * 1024 * 1024)
    return (size / (1024 * 1024)).toFixed(1) + " MB";

  return (size / (1024 * 1024 * 1024)).toFixed(1) + " GB";
}

function get_token() {
  var params = query_string();
  return params["token"];
}

function query_string() {
  // This function is anonymous, is executed immediately and 
  // the return value is assigned to QueryString!
  var query_string = {};
  var query = window.location.search.substring(1);
  var vars = query.split("&");
  for (var i=0;i<vars.length;i++) {
    var pair = vars[i].split("=");
        // If first entry with this name
    if (typeof query_string[pair[0]] === "undefined") {
      query_string[pair[0]] = decodeURIComponent(pair[1]);
        // If second entry with this name
    } else if (typeof query_string[pair[0]] === "string") {
      var arr = [ query_string[pair[0]],decodeURIComponent(pair[1]) ];
      query_string[pair[0]] = arr;
        // If third or later entry with this name
    } else {
      query_string[pair[0]].push(decodeURIComponent(pair[1]));
    }
  } 
  return query_string;
};

function check_token(min_level) {
  if (min_level === undefined)
    min_level = 2;

  var token = get_token();
  $.ajax("/test_token?token=" + token, {
    dataType: "json" 
  }).done(function(response) {
    if ((response.code === undefined) || (response.code == -1) || (response.code > min_level))
      window.location.href = "/index.html";

    switch (response.code) {
      case 0:
        $("#user-name").text("Admin");
        current_user_level = 0;
        break;
      case 1:
        $("#user-name").text("User level 1");
        current_user_level = 1;
        break;
      case 2:
        $("#user-name").text("User level 2");
        current_user_level = 2;
        $("#navbar-images").hide();
        $("#navbar-scheduler").hide();
        break;
    }      
  });
}