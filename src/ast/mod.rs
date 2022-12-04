mod ast;
pub mod region;
pub mod text;

use text::Text;

pub type ParsedAst<'i> = ast::File<ParsedContent<'i>>;

pub enum ParsedContent<'i> {
    Call {
        name: Text<'i>,
        args: Vec<ParsedContent<'i>>,
    },
    Word(Text<'i>),
    Whitespace(&'i str),
    Comment(&'i str),
    MultiLineComment(MultiLineComment<'i>),
}

pub enum MultiLineComment<'i> {
    Word(&'i str),
    Whitespace(&'i str),
    Indented(Box<MultiLineComment<'i>>),
    Nested(Box<MultiLineComment<'i>>),
}

