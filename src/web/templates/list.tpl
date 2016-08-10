<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
  </head>
  <body>
    <form method="POST" action="{{cgi:script_name}}/list.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <div id="navigation">
      </div>
      <div id="centerDoc">
        <h1>Circus: list</h1>
        <select name="name">
          {{#names}}
          <option value="{{item}}">{{item}}</option>
          {{/names}}
        </select>
        <input type="submit" name="action" value="show"/>
      </div>
    </form>
  </body>
</html>
