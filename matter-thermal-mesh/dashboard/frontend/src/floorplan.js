/**
 * floorplan.js — SVG Floor Plan Renderer
 *
 * Draws room outlines and sensor node positions on the floor plan SVG.
 * Rooms and positions are loaded from config.yaml.
 */

const FloorPlan = (() => {
  let svg;
  let floorConfig = {};
  let nodesConfig = [];

  function init(floorplan, nodes) {
    floorConfig = floorplan;
    nodesConfig = nodes;
    svg = d3.select("#floorplan-svg");
    render();
    window.addEventListener("resize", render);
  }

  function render() {
    svg.selectAll("*").remove();

    const container = document.getElementById("floorplan-container");
    const width = container.clientWidth;
    const height = container.clientHeight;

    svg.attr("viewBox", `0 0 ${floorConfig.width} ${floorConfig.height}`);

    // Draw rooms
    const rooms = svg.selectAll(".room")
      .data(floorConfig.rooms)
      .enter()
      .append("g")
      .attr("class", "room");

    // Room rectangles
    rooms.append("rect")
      .attr("x", d => d.x)
      .attr("y", d => d.y)
      .attr("width", d => d.width)
      .attr("height", d => d.height)
      .attr("fill", "none")
      .attr("stroke", "#2a2a3e")
      .attr("stroke-width", 2)
      .attr("rx", 4);

    // Room labels
    rooms.append("text")
      .attr("x", d => d.x + d.width / 2)
      .attr("y", d => d.y + 20)
      .attr("text-anchor", "middle")
      .attr("fill", "#555")
      .attr("font-size", "12px")
      .attr("font-family", "monospace")
      .text(d => d.name);

    // Draw sensor node markers
    const nodeMarkers = svg.selectAll(".node-marker")
      .data(nodesConfig)
      .enter()
      .append("g")
      .attr("class", "node-marker")
      .attr("transform", d =>
        `translate(${d.floor_x * floorConfig.width}, ${d.floor_y * floorConfig.height})`
      );

    // Node dot
    nodeMarkers.append("circle")
      .attr("r", 6)
      .attr("fill", "#00d4ff")
      .attr("stroke", "#fff")
      .attr("stroke-width", 1.5)
      .attr("opacity", 0.8);

    // Node label
    nodeMarkers.append("text")
      .attr("y", -12)
      .attr("text-anchor", "middle")
      .attr("fill", "#888")
      .attr("font-size", "10px")
      .attr("font-family", "monospace")
      .text(d => d.id);

    // Pulsing animation for active nodes
    nodeMarkers.append("circle")
      .attr("r", 6)
      .attr("fill", "none")
      .attr("stroke", "#00d4ff")
      .attr("stroke-width", 1)
      .attr("opacity", 0)
      .each(function() {
        const circle = d3.select(this);
        function pulse() {
          circle
            .attr("r", 6)
            .attr("opacity", 0.6)
            .transition()
            .duration(2000)
            .attr("r", 20)
            .attr("opacity", 0)
            .on("end", pulse);
        }
        pulse();
      });
  }

  return { init, render };
})();
