use crate::ast::{
    parsed::{Content, Sugar},
    Dash, Par, ParPart,
};

pub struct HtmlBuilder {
    content: String,
}

impl HtmlBuilder {
    pub fn new() -> Self {
        HtmlBuilder {
            content: String::new(),
        }
    }

    pub fn build_parpart(&mut self, par: &Par<ParPart<Content>>) {
        for y in par.parts.iter() {
            match y {
                crate::ast::ParPart::Line(cs) => {
                    for c in cs {
                        self.build(&c);
                        self.content.push_str("\n");
                    }
                }
                crate::ast::ParPart::Command(c) => {
                    self.build(&c);
                }
            }
        }
    }

    pub fn build(&mut self, content: &Content) {
        match content {
            Content::Command {
                name,
                // pluses,
                attrs,
                inline_args,
                remainder_arg,
                trailer_args,
                ..
            } => {
                self.content.push_str("<");
                self.content.push_str(name.as_str());
                if let Some(attrs) = attrs {
                    for arg in attrs.args() {
                        self.content.push_str(" ");
                        self.content.push_str(arg.name());
                        if let Some(value) = arg.value() {
                            self.content.push_str("=\"");
                            self.content.push_str(value);
                            self.content.push_str("\"");
                        }
                    }
                }
                self.content.push_str(">");
                for arg in inline_args.iter().flatten() {
                    self.build(arg);
                }
                if let Some(arg) = remainder_arg {
                    for a in arg {
                        self.build(a);
                    }
                }
                for arg in trailer_args.iter().flatten() {
                    self.build_parpart(arg);
                }
                self.content.push_str("</");
                self.content.push_str(name.as_str());
                self.content.push_str(">");
            }
            Content::Word { word, .. } => {
                self.content.push_str(word.as_str());
            }
            Content::Whitespace { whitespace, .. } => {
                self.content.push_str(whitespace);
            }
            Content::Sugar(sugar) => match sugar {
                Sugar::Italic { arg, .. } => {
                    self.content.push_str("<i>");
                    for c in arg {
                        self.build(c);
                    }
                    self.content.push_str("</i>");
                }
                Sugar::Bold { arg, .. } => {
                    self.content.push_str("<b>");
                    for c in arg {
                        self.build(c);
                    }
                    self.content.push_str("</b>");
                }
                Sugar::Monospace { arg, .. } => {
                    self.content.push_str("<code>");
                    for c in arg {
                        self.build(c);
                    }
                    self.content.push_str("</code>");
                }
                _ => {}
                // Sugar::Smallcaps { arg, loc } => todo!(),
                // Sugar::AlternateFace { arg, loc } => todo!(),
            },
            Content::Dash { dash, .. } => self.content.push_str(match dash {
                Dash::Hyphen => "-",
                Dash::En => "–",
                Dash::Em => "—",
            }),
            _ => {} // Content::Glue { glue, loc } => {}
                    // Content::Verbatim { verbatim, loc } => {}
                    // Content::Comment { comment, loc } => {}
                    // Content::MultiLineComment { content, loc } => {}
        }
    }

    pub fn complete(self) -> String {
        self.content
    }
}
