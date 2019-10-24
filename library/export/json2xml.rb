#!/usr/bin/env ruby

require 'nokogiri'
require 'json'

infile = ARGV[0]
outfile = infile.gsub('.json', '.xml')
json = JSON.parse(File.read(infile))

doc = Nokogiri::XML::Document.new
root = Nokogiri::XML::Node.new('dfhack', doc)
types = Nokogiri::XML::Node.new('types', doc)

json['types'].each_with_index do |type, idx|
  type_node = Nokogiri::XML::Node.new('type', doc)
  type_node['id'] = idx
  type.each do |key, value|
    if key != 'fields'
      type_node[key] = value
    else
      fields_node = Nokogiri::XML::Node.new('fields', doc)
      value.each do |field|
        field_node = Nokogiri::XML::Node.new('field', doc)
        field.each do |name, value|
          field_node[name] = value
        end
        fields_node << field_node
      end
      type_node << fields_node
    end
  end
  types << type_node
end
root << types

def namespace_to_xml(json, doc)
  namespace_node = Nokogiri::XML::Node.new('namespace', doc)
  namespace_node['name'] = json['name']
  labels_node = Nokogiri::XML::Node.new('labels', doc)
  json.fetch('labels', []).each do |label|
    label_node = Nokogiri::XML::Node.new('label', doc)
    label.each do |k, v|
      label_node[k] = v
    end
    labels_node << label_node
  end
  namespace_node << labels_node
  namespaces_node = Nokogiri::XML::Node.new('namespaces', doc)
  json.fetch('namespaces', []).each do |namespace|
    namespaces_node << namespace_to_xml(namespace, doc)
  end
  namespace_node << namespaces_node
  namespace_node
end

root << namespace_to_xml(json['namespace'], doc)
doc << root
File.write(outfile, doc.to_xml)
