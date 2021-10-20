<!-- top navigation bar -->
<div class="navbar navbar-default navbar-fixed-top" role="navigation" style="-webkit-app-region: drag; -webkit-user-select: none;">
  <div class="container">
    <div class="navbar-header">
      <button type="button" class="navbar-toggle collapsed" data-toggle="collapse" data-target=".navbar-collapse">
        <span class="sr-only">Toggle navigation</span>
        <span class="icon-bar"></span>
        <span class="icon-bar"></span>
        <span class="icon-bar"></span>
      </button>
      <a class="navbar-brand" href="#" data-toggle="modal" data-target="#about-box">Monitor Systems Control Center</a>  
    </div>
    <div class="navbar-collapse collapse">
      <ul class="nav navbar-nav">
        <li {{active-device-setup}}><a href="#" onclick="window.location.href='status.html?token=' + get_token()"><span class="glyphicon glyphicon-dashboard"></span> {{strong-device-setup}}Device Setup{{endstrong-device-setup}}</a></li>
        <li {{active-files}} id="navbar-files"><a href="#" onclick="window.location.href='files.html?token=' + get_token()"><span class="glyphicon glyphicon-folder-open"></span> {{strong-files}}Files View{{endstrong-files}}</a></li>
        <li {{active-images}} id="navbar-images"><a href="#" onclick="window.location.href='bitmaps.html?token=' + get_token()"><span class="glyphicon glyphicon-picture"></span> {{strong-images}}Images{{endstrong-images}}</a></li>
        <li {{active-scheduler}} id="navbar-scheduler"><a href="#" onclick="window.location.href='scheduler.html?token=' + get_token()"><span class="glyphicon glyphicon-calendar"></span> {{strong-scheduler}}Scheduler{{endstrong-scheduler}}</a></li>
        <li {{active-camera}} id="navbar-camera"><a href="#" onclick="window.location.href='camera.html?token=' + get_token()"><span class="glyphicon glyphicon-camera"></span> {{strong-camera}}Camera{{endstrong-camera}}</a></li>
      </ul>
      <ul class="nav navbar-nav navbar-right">
        <li><p class="navbar-text" style="color: #fff">Logged in as: <span id="user-name"></span></p></li>
      </ul>
    </div><!--/.nav-collapse -->
  </div>
</div>