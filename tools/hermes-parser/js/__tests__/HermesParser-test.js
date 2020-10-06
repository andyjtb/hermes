/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @format
 */

'use strict';

jest.disableAutomock();

const {parse} = require('../build/index');

/**
 * Utility for quickly creating source locations inline.
 */
function loc(startLine, startColumn, endLine, endColumn) {
  return {
    start: {
      line: startLine,
      column: startColumn,
    },
    end: {
      line: endLine,
      column: endColumn,
    },
  };
}

test('Can parse simple file', () => {
  expect(parse('const x = 1')).toMatchObject({
    type: 'Program',
    body: [
      expect.objectContaining({
        type: 'VariableDeclaration',
        kind: 'const',
        declarations: [
          expect.objectContaining({
            type: 'VariableDeclarator',
            id: expect.objectContaining({
              type: 'Identifier',
              name: 'x',
            }),
            init: expect.objectContaining({
              type: 'Literal',
              value: 1,
            }),
          }),
        ],
      }),
    ],
  });
});

test('Parse error is thrown', () => {
  expect(() => parse('const = 1')).toThrow(
    `1:6: 'identifier' expected in declaration
const = 1
~~~~~~^
`,
  );
});

test('Parse error does not include caret line for non-ASCII characters', () => {
  expect(() => parse('/*\u0176*/ const = 1')).toThrow(
    `1:13: 'identifier' expected in declaration
/*\u0176*/ const = 1
`,
  );
});

test('Parsing comments', () => {
  const source = '/*Block comment*/ 1; // Line comment';

  // ESTree comments in AST
  expect(parse(source)).toMatchObject({
    type: 'Program',
    comments: [
      {
        type: 'Block',
        value: 'Block comment',
        loc: {
          start: {
            line: 1,
            column: 0,
          },
          end: {
            line: 1,
            column: 17,
          },
        },
        range: [0, 17],
      },
      {
        type: 'Line',
        value: ' Line comment',
        loc: {
          start: {
            line: 1,
            column: 21,
          },
          end: {
            line: 1,
            column: 36,
          },
        },
        range: [21, 36],
      },
    ],
  });

  // Babel comments in AST
  expect(parse(source, {babel: true})).toMatchObject({
    type: 'File',
    program: {
      type: 'Program',
      body: [
        {
          type: 'ExpressionStatement',
        },
      ],
    },
    comments: [
      {
        type: 'Block',
        value: 'Block comment',
      },
      {
        type: 'Line',
        value: ' Line comment',
      },
    ],
  });
});

test('Babel root File node', () => {
  expect(parse('test', {babel: true})).toMatchObject({
    type: 'File',
    loc: loc(1, 0, 1, 4),
    start: 0,
    end: 4,
    program: {
      type: 'Program',
      loc: loc(1, 0, 1, 4),
      start: 0,
      end: 4,
      body: [
        {
          type: 'ExpressionStatement',
        },
      ],
      directives: [],
    },
    comments: [],
  });
});

test('Source locations', () => {
  // ESTree source locations include range
  expect(parse('Foo')).toMatchObject({
    type: 'Program',
    body: [
      {
        type: 'ExpressionStatement',
        loc: {
          start: {
            line: 1,
            column: 0,
          },
          end: {
            line: 1,
            column: 3,
          },
        },
        range: [0, 3],
      },
    ],
  });

  // Babel source locations include start/end properties
  expect(parse('Foo', {babel: true})).toMatchObject({
    type: 'File',
    program: {
      type: 'Program',
      body: [
        {
          type: 'ExpressionStatement',
          loc: {
            start: {
              line: 1,
              column: 0,
            },
            end: {
              line: 1,
              column: 3,
            },
          },
          start: 0,
          end: 3,
        },
      ],
    },
  });
});

test('Top level directives', () => {
  const source = `'use strict';
'use strict';
Foo;`;

  // ESTree top level directive nodes
  expect(parse(source)).toMatchObject({
    type: 'Program',
    body: [
      {
        type: 'ExpressionStatement',
        loc: loc(1, 0, 1, 13),
        expression: {
          type: 'Literal',
          value: 'use strict',
          loc: loc(1, 0, 1, 12),
        },
        directive: 'use strict',
      },
      {
        type: 'ExpressionStatement',
        loc: loc(2, 0, 2, 13),
        expression: {
          type: 'Literal',
          value: 'use strict',
          loc: loc(2, 0, 2, 12),
        },
        directive: 'use strict',
      },
      {
        type: 'ExpressionStatement',
        loc: loc(3, 0, 3, 4),
        expression: {
          type: 'Identifier',
          name: 'Foo',
          loc: loc(3, 0, 3, 3),
        },
      },
    ],
  });

  // Babel top level directive nodes
  expect(parse(source, {babel: true})).toMatchObject({
    type: 'File',
    program: {
      type: 'Program',
      body: [
        {
          type: 'ExpressionStatement',
          loc: loc(3, 0, 3, 4),
          expression: {
            type: 'Identifier',
            name: 'Foo',
            loc: loc(3, 0, 3, 3),
          },
        },
      ],
      directives: [
        {
          type: 'Directive',
          loc: loc(1, 0, 1, 13),
          value: {
            type: 'DirectiveLiteral',
            loc: loc(1, 0, 1, 12),
            value: 'use strict',
          },
        },
        {
          type: 'Directive',
          loc: loc(2, 0, 2, 13),
          value: {
            type: 'DirectiveLiteral',
            loc: loc(2, 0, 2, 12),
            value: 'use strict',
          },
        },
      ],
    },
  });
});

test('Function body directive', () => {
  const source = `function test() {
    'use strict';
    Foo;
  }
  `;

  // ESTree directive node in function body
  expect(parse(source)).toMatchObject({
    type: 'Program',
    body: [
      {
        type: 'FunctionDeclaration',
        body: {
          type: 'BlockStatement',
          body: [
            {
              type: 'ExpressionStatement',
              loc: loc(2, 4, 2, 17),
              expression: {
                type: 'Literal',
                value: 'use strict',
                loc: loc(2, 4, 2, 16),
              },
              directive: 'use strict',
            },
            {
              type: 'ExpressionStatement',
              loc: loc(3, 4, 3, 8),
              expression: {
                type: 'Identifier',
                name: 'Foo',
                loc: loc(3, 4, 3, 7),
              },
            },
          ],
        },
      },
    ],
  });

  // Babel directive node in function body
  expect(parse(source, {babel: true})).toMatchObject({
    type: 'File',
    program: {
      type: 'Program',
      body: [
        {
          type: 'FunctionDeclaration',
          body: {
            type: 'BlockStatement',
            body: [
              {
                type: 'ExpressionStatement',
                loc: loc(3, 4, 3, 8),
                expression: {
                  type: 'Identifier',
                  name: 'Foo',
                  loc: loc(3, 4, 3, 7),
                },
              },
            ],
            directives: [
              {
                type: 'Directive',
                loc: loc(2, 4, 2, 17),
                value: {
                  type: 'DirectiveLiteral',
                  loc: loc(2, 4, 2, 16),
                  value: 'use strict',
                },
              },
            ],
          },
        },
      ],
    },
  });
});

test('Literals', () => {
  const source = `
    null;
    10;
    "test";
    true;
    /foo/g;
  `;

  // ESTree AST literal nodes
  expect(parse(source)).toMatchObject({
    type: 'Program',
    body: [
      {
        type: 'ExpressionStatement',
        expression: {
          type: 'Literal',
          value: null,
        },
      },
      {
        type: 'ExpressionStatement',
        expression: {
          type: 'Literal',
          value: 10,
        },
      },
      {
        type: 'ExpressionStatement',
        expression: {
          type: 'Literal',
          value: 'test',
        },
      },
      {
        type: 'ExpressionStatement',
        expression: {
          type: 'Literal',
          value: true,
        },
      },
      {
        type: 'ExpressionStatement',
        expression: {
          type: 'Literal',
          value: new RegExp('foo', 'g'),
          regex: {
            pattern: 'foo',
            flags: 'g',
          },
        },
      },
    ],
  });

  // ESTree AST with invalid RegExp literal
  expect(parse('/foo/qq')).toMatchObject({
    type: 'Program',
    body: [
      {
        type: 'ExpressionStatement',
        expression: {
          type: 'Literal',
          value: null,
          regex: {
            pattern: 'foo',
            flags: 'qq',
          },
        },
      },
    ],
  });

  // Babel AST literal nodes
  expect(parse(source, {babel: true})).toMatchObject({
    type: 'File',
    program: {
      type: 'Program',
      body: [
        {
          type: 'ExpressionStatement',
          expression: {
            type: 'NullLiteral',
          },
        },
        {
          type: 'ExpressionStatement',
          expression: {
            type: 'NumericLiteral',
            value: 10,
          },
        },
        {
          type: 'ExpressionStatement',
          expression: {
            type: 'StringLiteral',
            value: 'test',
          },
        },
        {
          type: 'ExpressionStatement',
          expression: {
            type: 'BooleanLiteral',
            value: true,
          },
        },
        {
          type: 'ExpressionStatement',
          expression: {
            type: 'RegExpLiteral',
            pattern: 'foo',
            flags: 'g',
          },
        },
      ],
    },
  });
});

test('Arrays', () => {
  const source = 'const [a,,b] = [1,,2];';

  // ESTree AST array nodes
  expect(parse(source)).toMatchObject({
    type: 'Program',
    body: [
      {
        type: 'VariableDeclaration',
        declarations: [
          {
            type: 'VariableDeclarator',
            id: {
              type: 'ArrayPattern',
              elements: [
                {
                  type: 'Identifier',
                  name: 'a',
                },
                null,
                {
                  type: 'Identifier',
                  name: 'b',
                },
              ],
            },
            init: {
              type: 'ArrayExpression',
              elements: [
                {
                  type: 'Literal',
                  value: 1,
                },
                null,
                {
                  type: 'Literal',
                  value: 2,
                },
              ],
            },
          },
        ],
      },
    ],
  });

  // Babel AST array nodes
  expect(parse(source, {babel: true})).toMatchObject({
    type: 'File',
    program: {
      type: 'Program',
      body: [
        {
          type: 'VariableDeclaration',
          declarations: [
            {
              type: 'VariableDeclarator',
              id: {
                type: 'ArrayPattern',
                elements: [
                  {
                    type: 'Identifier',
                    name: 'a',
                  },
                  null,
                  {
                    type: 'Identifier',
                    name: 'b',
                  },
                ],
              },
              init: {
                type: 'ArrayExpression',
                elements: [
                  {
                    type: 'NumericLiteral',
                    value: 1,
                  },
                  null,
                  {
                    type: 'NumericLiteral',
                    value: 2,
                  },
                ],
              },
            },
          ],
        },
      ],
    },
  });
});
