<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
  </head>
  <body>
    <h1>Circus: list</h1>
    <form method="POST" action="{{cgi:script_name}}/list.do">
      <select name="name">
      {{#names}}
        <option value="{{item}}">{{item}}</option>
      {{/names}}
      </select>
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="submit" name="action" value="show"/>
    </form>
  </body>
</html>
