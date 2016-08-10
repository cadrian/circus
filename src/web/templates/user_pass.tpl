<html>
  <head>
    <title>Circus</title>
    <script src="{{config:static_path}}/clipboard.min.js"></script>
    <script type="text/javascript" src="{{config:static_path}}/circus.js"></script>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
  </head>
  <body>
    <form method="POST" action="{{cgi:script_name}}/user_home.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action" value="list"/>
      <div id="navigation">
      </div>
      <div id="centerDoc">
        <h1>Circus: user</h1>
        <h2>Your password</h2>
        <p>The password is loaded.</p>
        <table>
          <tr>
            <td>
              <b>Key:</b>
            </td>
            <td>
              {{key}}
            </td>
          </tr>
          <tr>
            <td>
              <b>Pass:</b>
            </td>
            <td>
              <div id="clip_message"><button class="clip" data-clipboard-text="{{pass}}" href="#" onclick="false;">Copy to clipboard</button></div>
            </td>
          </tr>
          <tr>
            <td colspan="2">
              <input type="submit" value="Back"/>
            </td>
          </tr>
        </table>
      </div>
    </form>
  </body>
</html>
